#pragma once
#include <Arduino.h>

// ============================================================
// AudioAnnouncer
// ------------------------------------------------------------
// Lê em voz alta o nome do produto e o preço, para acessibilidade.
//
// IMPORTANTE: síntese de voz (TTS) neural em tempo real NÃO é viável
// no ESP32 (recursos insuficientes de RAM/CPU para um modelo de
// qualidade aceitável). A abordagem usada aqui é "TTS por
// concatenação": frases são montadas juntando pequenos clipes de
// áudio PRÉ-GRAVADOS (PCM 16 bits, mono, 16kHz, sem cabeçalho) que
// ficam no LittleFS, um por palavra/número. Veja tools/generate_audio.py
// para gerar esses clipes a partir de um TTS rodando no computador
// (gTTS/pyttsx3), uma única vez, no momento de preparar o firmware.
//
// Se um clipe necessário não existir, o fallback é uma sequência de
// bipes no buzzer (apenas para indicar "algo mudou / verifique
// visualmente"), e um aviso é logado no Serial.
// ============================================================

class AudioAnnouncer {
public:
    void begin();

    // Fala "<nome do produto>, <preço> por <unidade>"
    void announcePrice(const char *productName, uint32_t priceCents, const char *unit);

    // Toca só um beep de confirmação (ex.: ao tocar o botão)
    void beepConfirm();

private:
    bool _i2sReady = false;

    bool playToken(const char *token);           // toca /audio/<token>.pcm se existir
    void playTokenList(const char **tokens, size_t count);
    void fallbackBeepPattern();

    // Converte um valor em centavos em uma lista de tokens de áudio em
    // português (ex.: 1999 -> "dezenove","reais","e","noventa","e","nove","centavos")
    size_t buildPriceTokens(uint32_t cents, const char **outTokens, size_t maxTokens);
};

extern AudioAnnouncer Audio;
