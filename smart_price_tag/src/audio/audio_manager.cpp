#include "audio_manager.h"
#include <esp_timer.h>

AudioManager* AudioManager::instance = nullptr;

AudioManager* AudioManager::getInstance() {
    if (!instance) {
        instance = new AudioManager();
    }
    return instance;
}

bool AudioManager::init() {
    Serial.println("Initializing Audio...");
    
    pinMode(AUDIO_BUZZER_PIN, OUTPUT);
    ledcSetup(0, 1000, 8);
    ledcAttachPin(AUDIO_BUZZER_PIN, 0);
    
    if (AUDIO_AMP_ENABLE != -1) {
        pinMode(AUDIO_AMP_ENABLE, OUTPUT);
        digitalWrite(AUDIO_AMP_ENABLE, HIGH);
    }
    
    audioInitialized = true;
    Serial.println("Audio initialized successfully!");
    return true;
}

void AudioManager::playPrice(float price, const char* productName) {
    if (!audioInitialized) return;
    
    char message[100];
    if (productName) {
        sprintf(message, "O preco do %s eh %.2f reais", productName, price);
    } else {
        sprintf(message, "Preco: %.2f reais", price);
    }
    
    playMessage(message);
}

void AudioManager::playProductName(const char* name) {
    if (!audioInitialized || !name) return;
    
    char message[100];
    sprintf(message, "Produto: %s", name);
    playMessage(message);
}

void AudioManager::playMessage(const char* message) {
    if (!audioInitialized || !message) return;
    
    Serial.printf("Speaking: %s\n", message);
    
    // Reprodução simplificada via tons
    const char* ptr = message;
    while (*ptr) {
        if (*ptr >= '0' && *ptr <= '9') {
            generateTone(800, 100);
            delay(50);
            generateTone(400, 100);
            delay(150);
        } else if ((*ptr >= 'A' && *ptr <= 'Z') || (*ptr >= 'a' && *ptr <= 'z')) {
            generateTone(600, 80);
            delay(50);
            generateTone(900, 80);
            delay(200);
        } else if (*ptr == ' ') {
            delay(300);
        }
        ptr++;
    }
    
    generateTone(1200, 200);
}

void AudioManager::generateTone(uint16_t frequency, uint16_t duration) {
    ledcWriteTone(0, frequency);
    delay(duration);
    ledcWrite(0, 0);
    delay(50);
}

void AudioManager::beep(uint16_t frequency, uint16_t duration) {
    generateTone(frequency, duration);
}

void AudioManager::setVolume(uint8_t vol) {
    volume = constrain(vol, 0, 100);
    uint8_t pwmValue = map(volume, 0, 100, 0, 255);
    ledcWrite(0, pwmValue);
}