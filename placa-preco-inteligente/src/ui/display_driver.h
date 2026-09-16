#pragma once
#include <Arduino.h>

// Inicializa TFT_eSPI, o touch XPT2046 e registra ambos no LVGL
// (driver de display + driver de entrada). Deve ser chamado após lv_init().
void display_init();

// Ativa/desativa o backlight (útil para economia de energia).
void display_set_backlight(bool on);

// Reinicia a calibração do touch gravada em NVS (uso em manutenção).
void touch_reset_calibration();
