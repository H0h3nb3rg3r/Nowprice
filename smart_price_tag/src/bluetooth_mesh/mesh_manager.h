#ifndef MESH_MANAGER_H
#define MESH_MANAGER_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

struct ProductData {
    char name[50];
    float price;
    char unit[20];
    uint32_t productCode;
    uint8_t discount;
};

class MeshManager {
public:
    static MeshManager* getInstance();
    
    bool init(const char* deviceName);
    void sendPriceUpdate(float price, const char* productName);
    void sendProductData(const ProductData& data);
    void setMessageCallback(void (*callback)(const uint8_t* data, size_t len));
    void processIncomingMessages();
    bool isMeshInitialized() const { return meshInitialized; }
    
private:
    MeshManager() : meshInitialized(false), messageCallback(nullptr) {}
    static MeshManager* instance;
    
    bool meshInitialized;
    void (*messageCallback)(const uint8_t* data, size_t len);
    
    static void meshNotifyCallback(BLERemoteCharacteristic* pChar, uint8_t* data, size_t len, bool isNotify);
};

#endif