#include "AudioAnnouncer.h"
#include <LittleFS.h>
#include <driver/i2s.h>
#include "../../include/config.h"

AudioAnnouncer Audio;

static const i2s_port_t I2S_PORT = I2S_NUM_0;

void AudioAnnouncer::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[Audio] Falha ao montar LittleFS - áudio pré-gravado indisponível.");
    }

    i2s_config_t cfg = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = AUDIO_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, // GPIO25 = DAC1 = canal direito
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    if (i2s_driver_install(I2S_PORT, &cfg, 0, nullptr) == ESP_OK) {
        i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN); // usa só GPIO25
        i2s_set_sample_rates(I2S_PORT, AUDIO_SAMPLE_RATE);
        _i2sReady = true;
    } else {
        Serial.println("[Audio] Falha ao iniciar I2S/DAC.");
        _i2sReady = false;
    }

    pinMode(BUZZER_PIN, OUTPUT);
}

// ------------------------------------------------------------
// Toca um clipe /audio/<token>.pcm do LittleFS via DAC.
// Retorna false se o arquivo não existir (chamador decide o fallback).
// ------------------------------------------------------------
bool AudioAnnouncer::playToken(const char *token) {
    if (!_i2sReady) return false;

    String path = String("/audio/") + token + ".pcm";
    if (!LittleFS.exists(path)) {
        Serial.printf("[Audio] Clipe ausente: %s\n", path.c_str());
        return false;
    }

    File f = LittleFS.open(path, "r");
    if (!f) return false;

    uint8_t buf[512];
    size_t n;
    while ((n = f.read(buf, sizeof(buf))) > 0) {
        size_t written = 0;
        i2s_write(I2S_PORT, buf, n, &written, portMAX_DELAY);
    }
    f.close();
    return true;
}

void AudioAnnouncer::playTokenList(const char **tokens, size_t count) {
    bool anyMissing = false;
    for (size_t i = 0; i < count; i++) {
        if (!playToken(tokens[i])) anyMissing = true;
    }
    if (anyMissing) fallbackBeepPattern();
}

void AudioAnnouncer::fallbackBeepPattern() {
    // Bipe simples via buzzer passivo (PWM) - só indica "atenção",
    // não substitui a leitura por voz. Gere os clipes reais com
    // tools/generate_audio.py para uma experiência completa.
    for (int i = 0; i < 2; i++) {
        tone(BUZZER_PIN, 1200, 120);
        delay(180);
    }
    noTone(BUZZER_PIN);
}

void AudioAnnouncer::beepConfirm() {
    tone(BUZZER_PIN, 1800, 60);
    delay(80);
    noTone(BUZZER_PIN);
}

// ------------------------------------------------------------
// Números por extenso em português (cobre 0-999 + milhar), token a
// token, batendo com os nomes de arquivo esperados em /audio/*.pcm.
// Vocabulário completo esperado (ver tools/generate_audio.py):
//   0..19, "vinte","trinta",...,"noventa" (dezenas),
//   "cem","cento","duzentos",...,"novecentos" (centenas),
//   "mil","e","reais","real","centavos","centavo"
// ------------------------------------------------------------
static const char *UNIDADES[] = {
    "zero","um","dois","tres","quatro","cinco","seis","sete","oito","nove",
    "dez","onze","doze","treze","catorze","quinze","dezesseis","dezessete","dezoito","dezenove"
};
static const char *DEZENAS[] = {
    "", "", "vinte","trinta","quarenta","cinquenta","sessenta","setenta","oitenta","noventa"
};
static const char *CENTENAS[] = {
    "", "cento","duzentos","trezentos","quatrocentos","quinhentos",
    "seiscentos","setecentos","oitocentos","novecentos"
};

// Adiciona os tokens de um número de 0 a 999. Retorna quantos tokens usou.
static size_t tokensForUpTo999(uint32_t n, const char **out, size_t maxTokens, size_t used) {
    if (n == 0) return used;

    if (n == 100) {
        if (used < maxTokens) out[used++] = "cem";
        return used;
    }

    uint32_t c = n / 100;
    uint32_t resto = n % 100;

    if (c > 0 && used < maxTokens) out[used++] = CENTENAS[c];
    if (c > 0 && resto > 0 && used < maxTokens) out[used++] = "e";

    if (resto > 0) {
        if (resto < 20) {
            if (used < maxTokens) out[used++] = UNIDADES[resto];
        } else {
            uint32_t d = resto / 10;
            uint32_t u = resto % 10;
            if (used < maxTokens) out[used++] = DEZENAS[d];
            if (u > 0) {
                if (used < maxTokens) out[used++] = "e";
                if (used < maxTokens) out[used++] = UNIDADES[u];
            }
        }
    }
    return used;
}

size_t AudioAnnouncer::buildPriceTokens(uint32_t cents, const char **outTokens, size_t maxTokens) {
    size_t used = 0;
    uint32_t reais = cents / 100;
    uint32_t centavos = cents % 100;

    if (reais == 0 && centavos == 0) {
        if (used < maxTokens) outTokens[used++] = "zero";
        if (used < maxTokens) outTokens[used++] = "reais";
        return used;
    }

    if (reais > 0) {
        if (reais >= 1000) {
            uint32_t milhar = reais / 1000;
            uint32_t restoReais = reais % 1000;
            used = tokensForUpTo999(milhar, outTokens, maxTokens, used);
            if (used < maxTokens) outTokens[used++] = "mil";
            if (restoReais > 0) {
                if (used < maxTokens) outTokens[used++] = "e";
                used = tokensForUpTo999(restoReais, outTokens, maxTokens, used);
            }
        } else {
            used = tokensForUpTo999(reais, outTokens, maxTokens, used);
        }
        if (used < maxTokens) outTokens[used++] = (reais == 1) ? "real" : "reais";
    }

    if (centavos > 0) {
        if (reais > 0 && used < maxTokens) outTokens[used++] = "e";
        used = tokensForUpTo999(centavos, outTokens, maxTokens, used);
        if (used < maxTokens) outTokens[used++] = (centavos == 1) ? "centavo" : "centavos";
    }

    return used;
}

void AudioAnnouncer::announcePrice(const char *productName, uint32_t priceCents, const char *unit) {
    // O nome do produto é falado como um único clipe pré-gravado
    // específico daquele produto: /audio/produtos/<productId>.pcm
    // (gerado no momento do provisionamento). Aqui usamos um clipe
    // genérico de fallback "produto" caso não exista um específico -
    // ajuste conforme seu fluxo de provisionamento.
    const char *nameTokens[] = { "produto_atual" }; // ver README: nome gravado por produto
    playTokenList(nameTokens, 1);

    const char *priceTokens[24];
    size_t n = buildPriceTokens(priceCents, priceTokens, 24);
    playTokenList(priceTokens, n);

    (void)productName;
    (void)unit;
}
