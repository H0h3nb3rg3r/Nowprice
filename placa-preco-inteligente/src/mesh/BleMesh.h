#pragma once
#include <Arduino.h>
#include <functional>

// ============================================================
// BleMesh
// ------------------------------------------------------------
// Implementa uma mesh BLE por "flood advertising com TTL":
//
//   1. Um dispositivo de origem (gateway, ou uma placa em modo admin)
//      monta um MeshPacket com um msgId único e ttl = MAX_TTL, e
//      começa a anunciá-lo (advertising BLE, não conectado).
//   2. Toda placa fica constantemente escaneando (BLE scan passivo/ativo).
//      Ao ver um MeshPacket novo (msgId ainda não visto):
//         - entrega o conteúdo para a aplicação (callback onPriceUpdate)
//         - se ttl > 0: decrementa o ttl e passa a anunciar o MESMO
//           pacote por um curto período (repetição), depois volta a
//           escanear. Isso propaga a mensagem salto a salto por toda
//           a loja, sem precisar de pareamento/conexão.
//   3. Um cache circular de msgIds recentes evita loops e retransmissões
//      duplicadas.
//
// Isso NÃO é o "Bluetooth Mesh" da especificação Bluetooth SIG (que exige
// uma pilha própria, pesada, e mal suportada em Arduino/ESP32 clássico).
// É uma mesh de flood mais simples, mas com a mesma propriedade prática
// que a loja precisa: cada placa é também um repetidor.
// ============================================================

enum MeshMsgType : uint8_t {
    MESH_MSG_PRICE_UPDATE = 0x01,
    MESH_MSG_PING         = 0x02,
};

#pragma pack(push, 1)
struct MeshPacket {
    uint8_t  magic;       // assinatura da nossa rede (evita colidir com outros BLE ao redor)
    uint32_t msgId;        // identificador único da mensagem, para dedup
    uint8_t  ttl;          // saltos restantes
    uint8_t  type;         // MeshMsgType
    uint16_t productId;    // produto alvo da atualização
    uint32_t priceCents;   // novo preço, em centavos
    uint8_t  flags;        // bit0 = promoção ativa
    uint8_t  checksum;     // XOR de todos os bytes anteriores
};
#pragma pack(pop)

using PriceUpdateCallback = std::function<void(uint16_t productId, uint32_t priceCents, bool promo)>;

// Payload recebido na característica de provisionamento (JSON).
using ProvisionCallback = std::function<void(const String &json)>;

class BleMesh {
public:
    void begin(uint16_t tagId);
    void loop(); // chamar continuamente no loop() principal

    void onPriceUpdate(PriceUpdateCallback cb) { _priceCb = cb; }
    void onProvision(ProvisionCallback cb)      { _provisionCb = cb; }

    // Usado por um dispositivo "gateway"/admin para originar uma
    // atualização de preço que será propagada pela mesh.
    void broadcastPriceUpdate(uint16_t productId, uint32_t priceCents, bool promo);

    // Alterna entre modo "mesh" (padrão, sempre escaneando/repetindo) e
    // modo "provisionamento" (fica conectável via GATT por N segundos
    // para receber a atribuição completa do produto desta placa).
    void enterProvisioningMode(uint32_t durationMs = 60000);
    bool isProvisioning() const { return _provisioning; }

    static uint8_t checksum(const MeshPacket &p);

private:
    uint16_t _tagId = 0;
    PriceUpdateCallback _priceCb;
    ProvisionCallback _provisionCb;

    // --- dedup cache ---
    uint32_t _seenIds[64];
    uint8_t  _seenCount = 0;
    uint8_t  _seenHead = 0;
    bool wasSeen(uint32_t msgId);
    void markSeen(uint32_t msgId);

    // --- repetição em andamento ---
    bool _repeating = false;
    MeshPacket _repeatPacket;
    uint32_t _repeatStartedAt = 0;

    // --- provisionamento ---
    bool _provisioning = false;
    uint32_t _provisioningUntil = 0;

    void startAdvertisingPacket(const MeshPacket &pkt, uint32_t durationMs);
    void stopAdvertising();
    void startScanning();

    void handleIncomingPacket(const MeshPacket &pkt);

    // callbacks internos do NimBLE (implementados em BleMesh.cpp via classes auxiliares)
    friend class MeshScanCallbacks;
    friend class MeshProvisionCharCallbacks;
};

extern BleMesh Mesh;
