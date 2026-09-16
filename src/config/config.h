#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Configurações de Rede
#define MESH_APP_KEY "SmartPriceMeshAppKey"
#define MESH_NET_KEY "SmartPriceMeshNetKey"
#define MESH_TX_POWER 10

// Configurações do Display
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define LVGL_TICK_PERIOD 10

// Pinos de Áudio
#define AUDIO_BUZZER_PIN 25
#define AUDIO_AMP_ENABLE 26
#define I2S_BCK_PIN 27
#define I2S_WS_PIN 14
#define I2S_DOUT_PIN 12

// Configurações do Produto
#define PRODUCT_NAME "Smart Price Tag"
#define PRODUCT_PRICE 99.99f
#define PRODUCT_UNIT "USD/kg"

// Tags de Mensagem
#define MSG_TYPE_PRICE_UPDATE 0x01
#define MSG_TYPE_PRODUCT_UPDATE 0x02
#define MSG_TYPE_SYNC 0x03

#endif