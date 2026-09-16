// ============================================================
//  Placa de Preço Inteligente - ESP32 CYD 2.8"
//
//  - Exibe nome, preço e informações do produto (LVGL)
//  - Recebe atualizações de preço por mesh BLE e REPETE o sinal
//    para as placas vizinhas (função repetidora)
//  - Lê o preço em voz alta ao toque (acessibilidade)
// ============================================================

#include <Arduino.h>
#include <lvgl.h>
#include <ArduinoJson.h>

#include "../include/config.h"
#include "ui/display_driver.h"
#include "ui/ui.h"
#include "mesh/BleMesh.h"
#include "audio/AudioAnnouncer.h"
#include "storage/ProductStore.h"

static ProductStore Store;

// Debounce do botão físico de acessibilidade
static uint32_t lastButtonPress = 0;
static const uint32_t BUTTON_DEBOUNCE_MS = 600;

// Detecção de "pressionar e segurar" no botão físico para entrar em
// modo de provisionamento (atribuir um produto a esta placa).
static uint32_t buttonHeldSince = 0;
static bool provisioningTriggered = false;
static const uint32_t PROVISION_HOLD_MS = 3000;

// ------------------------------------------------------------
// Acessibilidade: anuncia o produto atual em voz
// ------------------------------------------------------------
static void announceCurrentProduct() {
    ProductInfo info = Store.get();
    Audio.beepConfirm();
    Audio.announcePrice(info.name, info.priceCents, info.unit);
    Serial.printf("[Audio] Anunciando: %s - %lu centavos\n",
                  info.name, (unsigned long)info.priceCents);
}

// ------------------------------------------------------------
// Callback da mesh: chegou uma atualização de preço
// ------------------------------------------------------------
static void handlePriceUpdate(uint16_t productId, uint32_t priceCents, bool promo) {
    ui_pulse_mesh_indicator();

    // A BleMesh já cuidou de repetir o pacote para os vizinhos.
    // Aqui só verificamos se a atualização é para o produto DESTA placa.
    if (Store.applyPriceUpdate(productId, priceCents, promo)) {
        ui_update_product(Store.get());
        Serial.printf("[Mesh] Preço atualizado: produto %u -> %lu centavos%s\n",
                      productId, (unsigned long)priceCents, promo ? " (promo)" : "");
    } else {
        Serial.printf("[Mesh] Repetido update do produto %u (não é desta placa)\n", productId);
    }
}

// ------------------------------------------------------------
// Callback de provisionamento: recebeu um JSON com os dados do produto
//
// Formato esperado (escrito na característica GATT):
// {"id":1042,"nome":"Arroz Tipo 1 5kg","preco":2490,"unidade":"pacote","codigo":"7891234567890"}
// ------------------------------------------------------------
static void handleProvision(const String &json) {
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[Provision] JSON inválido: %s\n", err.c_str());
        return;
    }

    ProductInfo info;
    info.productId  = doc["id"] | 0;
    info.priceCents = doc["preco"] | 0;
    info.promo      = doc["promo"] | false;

    strlcpy(info.name, doc["nome"] | "Sem produto", sizeof(info.name));
    strlcpy(info.unit, doc["unidade"] | "un", sizeof(info.unit));
    strlcpy(info.code, doc["codigo"] | "", sizeof(info.code));

    Store.setFull(info);
    ui_update_product(Store.get());
    Audio.beepConfirm();

    Serial.printf("[Provision] Produto atribuído: %s (id %u)\n", info.name, info.productId);
}

// ------------------------------------------------------------
// Botão físico (GPIO) de acessibilidade
// ------------------------------------------------------------
static void pollPhysicalButton() {
    bool pressed = (digitalRead(ACCESS_BUTTON_PIN) == LOW); // botão com pull-up

    if (pressed) {
        if (buttonHeldSince == 0) buttonHeldSince = millis();

        // Segurar por 3s -> modo provisionamento
        if (!provisioningTriggered && (millis() - buttonHeldSince > PROVISION_HOLD_MS)) {
            provisioningTriggered = true;
            Mesh.enterProvisioningMode(60000);
            ui_set_provisioning_indicator(true);
            Audio.beepConfirm();
            Serial.println("[Provision] Modo de configuração ativo por 60s.");
        }
        return;
    }

    // Botão solto: se foi um toque curto, anuncia o preço
    if (buttonHeldSince != 0) {
        uint32_t heldFor = millis() - buttonHeldSince;
        buttonHeldSince = 0;

        if (!provisioningTriggered && heldFor < PROVISION_HOLD_MS) {
            if (millis() - lastButtonPress > BUTTON_DEBOUNCE_MS) {
                lastButtonPress = millis();
                announceCurrentProduct();
            }
        }
        provisioningTriggered = false;
    }
}

// ------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println("\n=== Placa de Preço Inteligente ===");

    pinMode(ACCESS_BUTTON_PIN, INPUT_PULLUP);

    // 1. Armazenamento (carrega o produto e o ID desta placa)
    Store.begin();
    Serial.printf("[Store] tagId desta placa: %u\n", Store.tagId());

    // 2. Display + LVGL
    lv_init();
    display_init();
    ui_init();
    ui_update_product(Store.get());
    ui_set_audio_button_callback(announceCurrentProduct);

    // 3. Áudio / acessibilidade
    Audio.begin();

    // 4. Mesh BLE (escaneia e repete continuamente)
    Mesh.onPriceUpdate(handlePriceUpdate);
    Mesh.onProvision(handleProvision);
    Mesh.begin(Store.tagId());

    Serial.println("[Setup] Pronto. Escutando a mesh...");
}

void loop() {
    lv_timer_handler();   // desenha a UI e processa eventos de toque
    Mesh.loop();          // alterna entre escanear e repetir pacotes
    pollPhysicalButton();

    // Mantém o indicador de provisionamento sincronizado com o estado real
    static bool lastProvState = false;
    bool provState = Mesh.isProvisioning();
    if (provState != lastProvState) {
        ui_set_provisioning_indicator(provState);
        lastProvState = provState;
    }

    delay(5);
}
