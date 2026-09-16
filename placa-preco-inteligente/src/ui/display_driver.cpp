#include "display_driver.h"
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include "../../include/config.h"

static TFT_eSPI tft = TFT_eSPI();

// Resolução lógica da UI. A placa é 240x320 (retrato).
static const uint16_t SCREEN_W = 240;
static const uint16_t SCREEN_H = 320;

// Buffer de desenho do LVGL: 1/8 da tela é um bom equilíbrio
// entre uso de RAM e fluidez.
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_W * SCREEN_H / 8];

// ------------------------------------------------------------
// Callback de flush: LVGL entrega uma área pronta, mandamos pro TFT
// ------------------------------------------------------------
static void disp_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(drv);
}

// ------------------------------------------------------------
// Calibração do touch resistivo.
// Os valores default abaixo funcionam na maioria das CYD em retrato,
// mas o toque resistivo varia de placa para placa. Se o botão não
// responder no lugar certo, rode o sketch de calibração do TFT_eSPI
// (exemplo "Touch_calibrate") e substitua estes números.
// ------------------------------------------------------------
static uint16_t calData[5] = { 300, 3500, 350, 3450, 2 };

static void load_touch_calibration() {
    Preferences p;
    p.begin(PREFS_NAMESPACE, true);
    size_t len = p.getBytesLength("touchCal");
    if (len == sizeof(calData)) {
        p.getBytes("touchCal", calData, sizeof(calData));
    }
    p.end();
    tft.setTouch(calData);
}

void touch_reset_calibration() {
    Preferences p;
    p.begin(PREFS_NAMESPACE, false);
    p.remove("touchCal");
    p.end();
}

// ------------------------------------------------------------
// Callback de leitura do touch para o LVGL
// ------------------------------------------------------------
static void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    uint16_t x = 0, y = 0;
    bool touched = tft.getTouch(&x, &y, 300); // 300 = limiar de pressão

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    data->state = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
}

// ------------------------------------------------------------
void display_init() {
    tft.begin();
    tft.setRotation(0); // 0 = retrato 240x320; use 2 se a imagem sair de cabeça para baixo
    tft.fillScreen(TFT_BLACK);

    pinMode(TFT_BL, OUTPUT);
    display_set_backlight(true);

    load_touch_calibration();

    lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, SCREEN_W * SCREEN_H / 8);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = SCREEN_W;
    disp_drv.ver_res  = SCREEN_H;
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_read_cb;
    lv_indev_drv_register(&indev_drv);
}

void display_set_backlight(bool on) {
    digitalWrite(TFT_BL, on ? TFT_BACKLIGHT_ON : !TFT_BACKLIGHT_ON);
}
