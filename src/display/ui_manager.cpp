#include "ui_manager.h"

UIManager* UIManager::instance = nullptr;

UIManager* UIManager::getInstance() {
    if (!instance) {
        instance = new UIManager();
    }
    return instance;
}

static void disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    TFT_eSPI* tft = &UIManager::getInstance()->tft;
    
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    
    tft->startWrite();
    tft->setAddrWindow(area->x1, area->y1, w, h);
    tft->pushColors((uint16_t*)color_p, w * h, true);
    tft->endWrite();
    
    lv_disp_flush_ready(disp);
}

static void touch_read(lv_indev_drv_t* indev, lv_indev_data_t* data) {
    uint16_t touchX, touchY;
    bool touched = UIManager::getInstance()->getTouch(&touchX, &touchY);
    
    if (touched) {
        data->point.x = touchX;
        data->point.y = touchY;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

bool UIManager::getTouch(uint16_t* x, uint16_t* y) {
    // Implementação simplificada - substituir por XPT2046 real
    *x = 120;
    *y = 160;
    return false;
}

bool UIManager::init() {
    Serial.println("Initializing Display...");
    
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    
    lv_init();
    
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[SCREEN_WIDTH * 50];
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * 50);
    
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = disp_flush;
    disp_drv.draw_buf = &draw_buf;
    display = lv_disp_drv_register(&disp_drv);
    
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read;
    touchIndev = lv_indev_drv_register(&indev_drv);
    
    createMainScreen();
    
    Serial.println("Display initialized successfully!");
    return true;
}

void UIManager::createMainScreen() {
    // Tela principal
    mainScreen = lv_obj_create(NULL);
    lv_scr_load(mainScreen);
    
    lv_obj_set_style_bg_color(mainScreen, lv_color_hex(0x1a1a2e), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(mainScreen, LV_OPA_COVER, LV_STATE_DEFAULT);
    
    // Header com nome do produto
    lv_obj_t* header = lv_obj_create(mainScreen);
    lv_obj_set_size(header, SCREEN_WIDTH, 50);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x16213e), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(header, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(header, 0, LV_STATE_DEFAULT);
    
    // Nome do produto
    productNameLabel = lv_label_create(header);
    lv_label_set_text(productNameLabel, "Smart Price Tag");
    lv_obj_set_style_text_color(productNameLabel, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(productNameLabel, &lv_font_montserrat_18, LV_STATE_DEFAULT);
    lv_obj_center(productNameLabel);
    
    // Container central do preço
    lv_obj_t* priceContainer = lv_obj_create(mainScreen);
    lv_obj_set_size(priceContainer, SCREEN_WIDTH - 40, 140);
    lv_obj_align(priceContainer, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_color(priceContainer, lv_color_hex(0x0f3460), LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(priceContainer, lv_color_hex(0x00ff88), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(priceContainer, 2, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(priceContainer, 15, LV_STATE_DEFAULT);
    
    // Preço principal
    priceLabel = lv_label_create(priceContainer);
    lv_label_set_text(priceLabel, "R$ 99.99");
    lv_obj_set_style_text_color(priceLabel, lv_color_hex(0x00ff88), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(priceLabel, &lv_font_montserrat_48, LV_STATE_DEFAULT);
    lv_obj_align(priceLabel, LV_ALIGN_CENTER, 0, -15);
    
    // Unidade
    unitLabel = lv_label_create(priceContainer);
    lv_label_set_text(unitLabel, "/ kg");
    lv_obj_set_style_text_color(unitLabel, lv_color_hex(0xaaaaaa), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(unitLabel, &lv_font_montserrat_16, LV_STATE_DEFAULT);
    lv_obj_align(unitLabel, LV_ALIGN_CENTER, 0, 40);
    
    // Desconto
    discountLabel = lv_label_create(mainScreen);
    lv_label_set_text(discountLabel, "");
    lv_obj_set_style_text_color(discountLabel, lv_color_hex(0xff4444), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(discountLabel, &lv_font_montserrat_16, LV_STATE_DEFAULT);
    lv_obj_align(discountLabel, LV_ALIGN_TOP_RIGHT, -15, 65);
    lv_obj_add_flag(discountLabel, LV_OBJ_FLAG_HIDDEN);
    
    // Código SKU
    codeLabel = lv_label_create(mainScreen);
    lv_label_set_text(codeLabel, "SKU: 000000");
    lv_obj_set_style_text_color(codeLabel, lv_color_hex(0x888888), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(codeLabel, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(codeLabel, LV_ALIGN_BOTTOM_LEFT, 15, -20);
    
    // Status da rede
    statusLabel = lv_label_create(mainScreen);
    lv_label_set_text(statusLabel, LV_SYMBOL_BLUETOOTH " Online");
    lv_obj_set_style_text_color(statusLabel, lv_color_hex(0x00ff88), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(statusLabel, &lv_font_montserrat_12, LV_STATE_DEFAULT);
    lv_obj_align(statusLabel, LV_ALIGN_BOTTOM_LEFT, 15, -5);
    
    // Botão de áudio
    audioButton = lv_btn_create(mainScreen);
    lv_obj_set_size(audioButton, 70, 70);
    lv_obj_align(audioButton, LV_ALIGN_BOTTOM_RIGHT, -15, -15);
    lv_obj_set_style_bg_color(audioButton, lv_color_hex(0x4a90e2), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(audioButton, lv_color_hex(0x357abd), LV_STATE_PRESSED);
    lv_obj_set_style_radius(audioButton, 35, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(audioButton, 10, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_x(audioButton, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_ofs_y(audioButton, 4, LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_color(audioButton, lv_color_hex(0x000000), LV_STATE_DEFAULT);
    
    lv_obj_t* audioIcon = lv_label_create(audioButton);
    lv_label_set_text(audioIcon, LV_SYMBOL_VOLUME_MAX);
    lv_obj_set_style_text_color(audioIcon, lv_color_hex(0xffffff), LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(audioIcon, &lv_font_montserrat_28, LV_STATE_DEFAULT);
    lv_obj_center(audioIcon);
    
    lv_obj_add_event_cb(audioButton, audioClickEvent, LV_EVENT_CLICKED, NULL);
}

void UIManager::updatePrice(float price, const char* productName) {
    updatePriceDisplay(price);
    if (productName) {
        updateNameDisplay(productName);
    }
}

void UIManager::updatePriceDisplay(float price) {
    char priceStr[20];
    sprintf(priceStr, "R$ %.2f", price);
    lv_label_set_text(priceLabel, priceStr);
}

void UIManager::updateNameDisplay(const char* name) {
    lv_label_set_text(productNameLabel, name);
}

void UIManager::updateUnitDisplay(const char* unit) {
    lv_label_set_text(unitLabel, unit);
}

void UIManager::updateCodeDisplay(uint32_t code) {
    char codeStr[20];
    sprintf(codeStr, "SKU: %06lu", code);
    lv_label_set_text(codeLabel, codeStr);
}

void UIManager::updateDiscountDisplay(uint8_t discount) {
    if (discount > 0) {
        char discStr[20];
        sprintf(discStr, "%d%% OFF", discount);
        lv_label_set_text(discountLabel, discStr);
        lv_obj_clear_flag(discountLabel, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(discountLabel, LV_OBJ_FLAG_HIDDEN);
    }
}

void UIManager::updateProductInfo(const char* name, const char* unit, uint32_t code) {
    if (name) updateNameDisplay(name);
    if (unit) updateUnitDisplay(unit);
    if (code) updateCodeDisplay(code);
}

void UIManager::updateDiscount(uint8_t discount) {
    updateDiscountDisplay(discount);
}

void UIManager::setAudioButtonCallback(void (*callback)()) {
    audioCallback = callback;
}

void UIManager::setTouchCallback(void (*callback)(uint16_t, uint16_t)) {
    touchCallback = callback;
}

void UIManager::audioClickEvent(lv_event_t* e) {
    if (instance && instance->audioCallback) {
        instance->audioCallback();
    }
}

void UIManager::processTouch() {
    // Processado pelo LVGL automaticamente
}

void UIManager::loop() {
    lv_timer_handler();
    lv_tick_inc(LVGL_TICK_PERIOD);
}