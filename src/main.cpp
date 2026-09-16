#include <Arduino.h>
#include "config/config.h"
#include "bluetooth_mesh/mesh_manager.h"
#include "display/ui_manager.h"
#include "audio/audio_manager.h"

MeshManager* mesh = nullptr;
UIManager* ui = nullptr;
AudioManager* audio = nullptr;

struct LocalProduct {
    char name[50];
    float price;
    char unit[20];
    uint32_t code;
    uint8_t discount;
} currentProduct = {
    "Cafe Especial",
    99.99f,
    "/ kg",
    123456,
    0
};

void onMeshMessageReceived(const uint8_t* data, size_t len) {
    if (len < 1) return;
    
    uint8_t msgType = data[0];
    size_t offset = 1;
    
    switch (msgType) {
        case MSG_TYPE_PRICE_UPDATE: {
            if (len < offset + sizeof(float)) break;
            float newPrice;
            memcpy(&newPrice, &data[offset], sizeof(float));
            offset += sizeof(float);
            
            if (offset + sizeof(uint16_t) <= len) {
                uint16_t nameLen;
                memcpy(&nameLen, &data[offset], sizeof(uint16_t));
                offset += sizeof(uint16_t);
                
                if (offset + nameLen <= len) {
                    char productName[50];
                    memcpy(productName, &data[offset], nameLen);
                    productName[nameLen] = '\0';
                    
                    ui->updatePrice(newPrice, productName);
                    strcpy(currentProduct.name, productName);
                    currentProduct.price = newPrice;
                    
                    Serial.printf("Price updated: %.2f - %s\n", newPrice, productName);
                }
            }
            break;
        }
        
        case MSG_TYPE_PRODUCT_UPDATE: {
            if (len < offset + sizeof(float) + sizeof(uint8_t) + sizeof(uint32_t)) break;
            
            ProductData product;
            memcpy(&product.price, &data[offset], sizeof(float));
            offset += sizeof(float);
            memcpy(&product.discount, &data[offset], sizeof(uint8_t));
            offset += sizeof(uint8_t);
            memcpy(&product.productCode, &data[offset], sizeof(uint32_t));
            offset += sizeof(uint32_t);
            
            if (offset + sizeof(uint16_t) <= len) {
                uint16_t nameLen;
                memcpy(&nameLen, &data[offset], sizeof(uint16_t));
                offset += sizeof(uint16_t);
                
                if (offset + nameLen <= len) {
                    memcpy(product.name, &data[offset], nameLen);
                    product.name[nameLen] = '\0';
                    
                    ui->updateProductInfo(product.name, " / kg", product.productCode);
                    ui->updateDiscount(product.discount);
                    
                    strcpy(currentProduct.name, product.name);
                    currentProduct.price = product.price;
                    currentProduct.code = product.productCode;
                    currentProduct.discount = product.discount;
                    
                    Serial.printf("Product updated: %s - %.2f\n", product.name, product.price);
                }
            }
            break;
        }
    }
}

void onAudioButtonPressed() {
    Serial.println("Audio button pressed!");
    audio->playPrice(currentProduct.price, currentProduct.name);
}

void onTouchDetected(uint16_t x, uint16_t y) {
    Serial.printf("Touch at: %d, %d\n", x, y);
}

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Smart Price Tag System ===");
    Serial.println("Initializing...");
    
    audio = AudioManager::getInstance();
    if (!audio->init()) {
        Serial.println("Audio initialization failed!");
    }
    
    ui = UIManager::getInstance();
    if (!ui->init()) {
        Serial.println("Display initialization failed!");
    }
    
    ui->setAudioButtonCallback(onAudioButtonPressed);
    ui->setTouchCallback(onTouchDetected);
    
    mesh = MeshManager::getInstance();
    if (!mesh->init("PriceTag_001")) {
        Serial.println("Mesh initialization failed!");
    }
    mesh->setMessageCallback(onMeshMessageReceived);
    
    ui->updatePrice(currentProduct.price, currentProduct.name);
    ui->updateProductInfo(currentProduct.name, currentProduct.unit, currentProduct.code);
    ui->updateDiscount(currentProduct.discount);
    
    audio->beep(1000, 200);
    delay(100);
    audio->beep(1500, 200);
    
    Serial.println("System ready!");
    Serial.println("================================");
}

void loop() {
    if (ui) {
        ui->loop();
    }
    
    if (mesh && mesh->isMeshInitialized()) {
        mesh->processIncomingMessages();
        
        static unsigned long lastHeartbeat = 0;
        if (millis() - lastHeartbeat > 30000) {
            lastHeartbeat = millis();
            mesh->sendPriceUpdate(currentProduct.price, currentProduct.name);
            Serial.println("Heartbeat sent");
        }
    }
    
    static unsigned long lastUpdate = 0;
    if (millis() - lastUpdate > 60000) {
        lastUpdate = millis();
        
        float newPrice = 50.0f + random(0, 100) / 10.0f;
        currentProduct.price = newPrice;
        
        ui->updatePrice(newPrice, currentProduct.name);
        mesh->sendPriceUpdate(newPrice, currentProduct.name);
        
        Serial.printf("Auto price update: %.2f\n", newPrice);
    }
    
    delay(10);
}