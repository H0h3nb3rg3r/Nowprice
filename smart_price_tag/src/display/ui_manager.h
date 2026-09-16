#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "../config/config.h"

class UIManager {
public:
    static UIManager* getInstance();
    
    bool init();
    void updatePrice(float price, const char* productName);
    void updateProductInfo(const char* name, const char* unit, uint32_t code);
    void updateDiscount(uint8_t discount);
    void setAudioButtonCallback(void (*callback)());
    void setTouchCallback(void (*callback)(uint16_t x, uint16_t y));
    void processTouch();
    void loop();
    bool getTouch(uint16_t* x, uint16_t* y);
    
    TFT_eSPI tft;
    
private:
    UIManager() : audioCallback(nullptr), touchCallback(nullptr) {}
    static UIManager* instance;
    
    lv_disp_t* display;
    lv_indev_t* touchIndev;
    
    lv_obj_t* mainScreen;
    lv_obj_t* productNameLabel;
    lv_obj_t* priceLabel;
    lv_obj_t* unitLabel;
    lv_obj_t* codeLabel;
    lv_obj_t* discountLabel;
    lv_obj_t* audioButton;
    lv_obj_t* statusLabel;
    
    void createMainScreen();
    void updatePriceDisplay(float price);
    void updateNameDisplay(const char* name);
    void updateUnitDisplay(const char* unit);
    void updateCodeDisplay(uint32_t code);
    void updateDiscountDisplay(uint8_t discount);
    
    void (*audioCallback)();
    void (*touchCallback)(uint16_t, uint16_t);
    
    static void audioClickEvent(lv_event_t* e);
};

#endif