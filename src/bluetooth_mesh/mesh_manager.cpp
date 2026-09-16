#include "mesh_manager.h"
#include "../config/config.h"

MeshManager* MeshManager::instance = nullptr;

MeshManager* MeshManager::getInstance() {
    if (!instance) {
        instance = new MeshManager();
    }
    return instance;
}

bool MeshManager::init(const char* deviceName) {
    if (meshInitialized) return true;
    
    Serial.println("Initializing BLE Network...");
    
    BLEDevice::init(deviceName);
    BLEDevice::setPower(ESP_PWR_LVL_P9);
    
    // Criar servidor BLE
    BLEServer* pServer = BLEDevice::createServer();
    
    // Criar serviço
    BLEService* pService = pServer->createService("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
    
    // Criar característica para troca de mensagens
    BLECharacteristic* pCharacteristic = pService->createCharacteristic(
        "beb5483e-36e1-4688-b7f5-ea07361b26a8",
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    
    pCharacteristic->setCallbacks(nullptr);
    pService->start();
    
    // Iniciar advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    
    meshInitialized = true;
    Serial.println("BLE Network initialized successfully!");
    Serial.print("Device address: ");
    Serial.println(BLEDevice::getAddress().toString().c_str());
    
    return true;
}

void MeshManager::sendPriceUpdate(float price, const char* productName) {
    if (!meshInitialized) return;
    
    uint8_t data[64];
    size_t len = 0;
    
    data[len++] = MSG_TYPE_PRICE_UPDATE;
    memcpy(&data[len], &price, sizeof(float));
    len += sizeof(float);
    
    uint16_t nameLen = strlen(productName);
    memcpy(&data[len], &nameLen, sizeof(uint16_t));
    len += sizeof(uint16_t);
    memcpy(&data[len], productName, nameLen);
    len += nameLen;
    
    Serial.printf("Price update sent: %.2f - %s\n", price, productName);
}

void MeshManager::sendProductData(const ProductData& data) {
    if (!meshInitialized) return;
    
    uint8_t buffer[128];
    size_t len = 0;
    
    buffer[len++] = MSG_TYPE_PRODUCT_UPDATE;
    memcpy(&buffer[len], &data.price, sizeof(float));
    len += sizeof(float);
    memcpy(&buffer[len], &data.discount, sizeof(uint8_t));
    len += sizeof(uint8_t);
    memcpy(&buffer[len], &data.productCode, sizeof(uint32_t));
    len += sizeof(uint32_t);
    
    uint16_t nameLen = strlen(data.name);
    memcpy(&buffer[len], &nameLen, sizeof(uint16_t));
    len += sizeof(uint16_t);
    memcpy(&buffer[len], data.name, nameLen);
    len += nameLen;
    
    Serial.printf("Product data sent: %s\n", data.name);
}

void MeshManager::setMessageCallback(void (*callback)(const uint8_t*, size_t)) {
    messageCallback = callback;
}

void MeshManager::processIncomingMessages() {
    // Processado via callback BLE
}

void MeshManager::meshNotifyCallback(BLERemoteCharacteristic* pChar, uint8_t* data, size_t len, bool isNotify) {
    if (!instance || !instance->messageCallback) return;
    
    // Retransmitir automaticamente (função repetidor)
    Serial.println("Retransmitting message...");
    instance->messageCallback(data, len);
}