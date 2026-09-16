#include "BleMesh.h"
#include <NimBLEDevice.h>
#include "../../include/config.h"

BleMesh Mesh;

static const uint8_t MESH_MAGIC = 0xA5;

// UUIDs do serviço de provisionamento (gerados aleatoriamente para este projeto;
// mantenha os mesmos em todas as placas e no app/gateway de administração).
static const char *PROVISION_SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
static const char *PROVISION_CHAR_UUID    = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";

static NimBLEScan       *g_scan        = nullptr;
static NimBLEAdvertising *g_advertising = nullptr;
static NimBLEServer      *g_server      = nullptr;

// ------------------------------------------------------------
// Checksum simples (XOR) - suficiente para detectar corrupção
// grosseira do pacote; não é criptografia nem autenticação.
// Para um ambiente de produção real, adicione um MAC (ex.: HMAC
// truncado) usando uma chave pré-compartilhada entre as placas.
// ------------------------------------------------------------
uint8_t BleMesh::checksum(const MeshPacket &p) {
    const uint8_t *b = reinterpret_cast<const uint8_t *>(&p);
    uint8_t x = 0;
    for (size_t i = 0; i < sizeof(MeshPacket) - 1; i++) x ^= b[i]; // exclui o próprio checksum
    return x;
}

// ------------------------------------------------------------
// Callbacks de scan: chamados pelo NimBLE a cada advertisement recebido
// ------------------------------------------------------------
class MeshScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
public:
    void onResult(NimBLEAdvertisedDevice *dev) override {
        if (!dev->haveManufacturerData()) return;

        std::string mfg = dev->getManufacturerData();
        // 2 bytes de company ID + payload do MeshPacket
        if (mfg.size() != 2 + sizeof(MeshPacket)) return;

        uint16_t companyId = (uint8_t)mfg[0] | ((uint8_t)mfg[1] << 8);
        if (companyId != BLE_MANUFACTURER_ID) return;

        MeshPacket pkt;
        memcpy(&pkt, mfg.data() + 2, sizeof(MeshPacket));

        if (pkt.magic != MESH_MAGIC) return;
        if (BleMesh::checksum(pkt) != pkt.checksum) return; // pacote corrompido, descarta

        Mesh.handleIncomingPacket(pkt);
    }
};
static MeshScanCallbacks g_scanCallbacks;

// ------------------------------------------------------------
// Callback da característica GATT de provisionamento
// ------------------------------------------------------------
class MeshProvisionCharCallbacks : public NimBLECharacteristicCallbacks {
public:
    void onWrite(NimBLECharacteristic *chr) override {
        std::string v = chr->getValue();
        if (v.empty()) return;
        if (Mesh._provisionCb) {
            Mesh._provisionCb(String(v.c_str()));
        }
    }
};
static MeshProvisionCharCallbacks g_provisionCallbacks;

// ------------------------------------------------------------
// begin()
// ------------------------------------------------------------
void BleMesh::begin(uint16_t tagId) {
    _tagId = tagId;

    String devName = String(DEVICE_NAME_PREFIX) + "-" + String(_tagId);
    NimBLEDevice::init(devName.c_str());
    NimBLEDevice::setPower(ESP_PWR_LVL_P6); // alcance razoável sem consumir demais

    // Servidor GATT (usado só durante o modo de provisionamento, mas já
    // deixamos o serviço registrado para simplificar).
    g_server = NimBLEDevice::createServer();
    NimBLEService *service = g_server->createService(PROVISION_SERVICE_UUID);
    NimBLECharacteristic *provChar = service->createCharacteristic(
        PROVISION_CHAR_UUID,
        NIMBLE_PROPERTY::WRITE
    );
    provChar->setCallbacks(&g_provisionCallbacks);
    service->start();

    g_advertising = NimBLEDevice::getAdvertising();

    g_scan = NimBLEDevice::getScan();
    g_scan->setAdvertisedDeviceCallbacks(&g_scanCallbacks, false);
    g_scan->setActiveScan(true);
    g_scan->setInterval((BLE_MESH_SCAN_INTERVAL_MS * 1000) / 625); // unidades de 0.625ms
    g_scan->setWindow((BLE_MESH_SCAN_WINDOW_MS * 1000) / 625);

    startScanning();

    memset(_seenIds, 0, sizeof(_seenIds));
    randomSeed(esp_random());
}

void BleMesh::startScanning() {
    if (g_scan && !g_scan->isScanning()) {
        // duration=0 -> escaneia indefinidamente (interrompido manualmente quando
        // precisamos anunciar uma repetição).
        g_scan->start(0, nullptr, false);
    }
}

void BleMesh::stopAdvertising() {
    if (g_advertising) g_advertising->stop();
}

// ------------------------------------------------------------
// Monta o payload de manufacturer data e começa a anunciar
// ------------------------------------------------------------
void BleMesh::startAdvertisingPacket(const MeshPacket &pkt, uint32_t durationMs) {
    if (g_scan && g_scan->isScanning()) g_scan->stop();

    std::string mfg;
    mfg += (char)(BLE_MANUFACTURER_ID & 0xFF);
    mfg += (char)((BLE_MANUFACTURER_ID >> 8) & 0xFF);
    mfg.append(reinterpret_cast<const char *>(&pkt), sizeof(MeshPacket));

    NimBLEAdvertisementData advData;
    advData.setFlags(0x06); // LE General Discoverable, BR/EDR não suportado
    advData.setManufacturerData(mfg);

    g_advertising->setAdvertisementData(advData);
    g_advertising->setScanResponseData(NimBLEAdvertisementData()); // vazio
    g_advertising->start();

    _repeating = true;
    _repeatPacket = pkt;
    _repeatStartedAt = millis();
    (void)durationMs; // duração controlada em loop() via BLE_MESH_ADV_DURATION_MS
}

// ------------------------------------------------------------
// Processa um pacote recém-recebido (nosso ou de outra placa)
// ------------------------------------------------------------
void BleMesh::handleIncomingPacket(const MeshPacket &pkt) {
    if (wasSeen(pkt.msgId)) return; // já vimos essa mensagem, ignora (evita loop)
    markSeen(pkt.msgId);

    if (pkt.type == MESH_MSG_PRICE_UPDATE) {
        bool promo = pkt.flags & 0x01;
        if (_priceCb) _priceCb(pkt.productId, pkt.priceCents, promo);
    }

    if (pkt.ttl > 0) {
        MeshPacket repeat = pkt;
        repeat.ttl -= 1;
        repeat.checksum = checksum(repeat);
        startAdvertisingPacket(repeat, BLE_MESH_ADV_DURATION_MS);
    }
}

// ------------------------------------------------------------
// dedup cache (buffer circular simples)
// ------------------------------------------------------------
bool BleMesh::wasSeen(uint32_t msgId) {
    for (uint8_t i = 0; i < _seenCount; i++) {
        if (_seenIds[i] == msgId) return true;
    }
    return false;
}

void BleMesh::markSeen(uint32_t msgId) {
    _seenIds[_seenHead] = msgId;
    _seenHead = (_seenHead + 1) % BLE_MESH_DEDUP_CACHE;
    if (_seenCount < BLE_MESH_DEDUP_CACHE) _seenCount++;
}

// ------------------------------------------------------------
// Origina uma atualização de preço (chamado por um gateway/admin)
// ------------------------------------------------------------
void BleMesh::broadcastPriceUpdate(uint16_t productId, uint32_t priceCents, bool promo) {
    MeshPacket pkt{};
    pkt.magic      = MESH_MAGIC;
    pkt.msgId      = ((uint32_t)esp_random());
    pkt.ttl        = BLE_MESH_MAX_TTL;
    pkt.type       = MESH_MSG_PRICE_UPDATE;
    pkt.productId  = productId;
    pkt.priceCents = priceCents;
    pkt.flags      = promo ? 0x01 : 0x00;
    pkt.checksum   = checksum(pkt);

    markSeen(pkt.msgId); // não repetir a própria origem quando ouvirmos ela de volta
    startAdvertisingPacket(pkt, BLE_MESH_ADV_DURATION_MS);
}

// ------------------------------------------------------------
// loop(): máquina de estados não bloqueante
// ------------------------------------------------------------
void BleMesh::loop() {
    if (_repeating && (millis() - _repeatStartedAt >= BLE_MESH_ADV_DURATION_MS)) {
        stopAdvertising();
        _repeating = false;
        startScanning();
    }

    if (_provisioning && millis() > _provisioningUntil) {
        _provisioning = false;
        // volta ao modo mesh normal (não conectável, só flood)
        stopAdvertising();
        startScanning();
    }
}

// ------------------------------------------------------------
// Modo de provisionamento: fica conectável por um tempo para que
// um app/gateway grave o produto desta placa via GATT.
// ------------------------------------------------------------
void BleMesh::enterProvisioningMode(uint32_t durationMs) {
    if (g_scan && g_scan->isScanning()) g_scan->stop();
    stopAdvertising();

    g_advertising->reset();
    g_advertising->addServiceUUID(PROVISION_SERVICE_UUID);
    g_advertising->setName(true);
    g_advertising->start();

    _provisioning = true;
    _provisioningUntil = millis() + durationMs;
}
