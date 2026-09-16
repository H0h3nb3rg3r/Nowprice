// ============================================================
// gateway_example.cpp
//
// Exemplo de dispositivo "gateway": um ESP32 (pode ser outra CYD, ou
// um ESP32 simples sem display) que ORIGINA as atualizações de preço.
// As placas da loja recebem e repetem o sinal automaticamente.
//
// Como usar:
//   1. Crie um segundo projeto PlatformIO (ou um env separado).
//   2. Copie este arquivo como src/main.cpp desse projeto, junto com
//      a pasta src/mesh/ e include/config.h deste repositório.
//   3. Grave em um ESP32 e envie comandos pelo Monitor Serial:
//
//        1042 2490          -> produto 1042, R$ 24,90
//        1042 1990 promo    -> produto 1042, R$ 19,90, marcado como promoção
//
// Em produção, este gateway normalmente ficaria conectado ao Wi-Fi e
// buscaria os preços do ERP da loja em vez de ler do Serial.
// ============================================================

#include <Arduino.h>
#include "mesh/BleMesh.h"
#include "../include/config.h"

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n=== Gateway de Preços (mesh BLE) ===");
    Serial.println("Formato: <productId> <precoEmCentavos> [promo]");
    Serial.println("Exemplo:  1042 2490");

    Mesh.begin(0xFFFE); // tagId reservado para o gateway
}

void loop() {
    Mesh.loop();

    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) return;

        int sp1 = line.indexOf(' ');
        if (sp1 < 0) {
            Serial.println("Formato inválido. Use: <id> <centavos> [promo]");
            return;
        }

        uint16_t productId = line.substring(0, sp1).toInt();
        String rest = line.substring(sp1 + 1);
        rest.trim();

        int sp2 = rest.indexOf(' ');
        uint32_t cents;
        bool promo = false;

        if (sp2 < 0) {
            cents = rest.toInt();
        } else {
            cents = rest.substring(0, sp2).toInt();
            promo = rest.substring(sp2 + 1).indexOf("promo") >= 0;
        }

        Mesh.broadcastPriceUpdate(productId, cents, promo);
        Serial.printf("-> Enviado: produto %u = %lu centavos%s\n",
                      productId, (unsigned long)cents, promo ? " (promo)" : "");
    }

    delay(5);
}
