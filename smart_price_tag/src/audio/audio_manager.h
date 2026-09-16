#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include <Arduino.h>
#include "../config/config.h"

class AudioManager {
public:
    static AudioManager* getInstance();
    
    bool init();
    void playPrice(float price, const char* productName);
    void playProductName(const char* name);
    void playMessage(const char* message);
    void setVolume(uint8_t volume);
    void beep(uint16_t frequency, uint16_t duration);
    
private:
    AudioManager() : volume(80), audioInitialized(false) {}
    static AudioManager* instance;
    
    uint8_t volume;
    bool audioInitialized;
    
    void generateTone(uint16_t frequency, uint16_t duration);
};

#endif