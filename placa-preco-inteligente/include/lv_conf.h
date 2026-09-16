/**
 * lv_conf.h - Configuração LVGL v8 para a Placa de Preço Inteligente
 *
 * Só definimos o que difere do padrão: lv_conf_internal.h aplica valores
 * default (via #ifndef) para tudo que não estiver aqui.
 */
#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* --- Cor --- */
#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   1   /* necessário para TFT_eSPI com pushColors() */

/* --- Memória --- */
#define LV_MEM_CUSTOM      0
#define LV_MEM_SIZE        (48U * 1024U)   /* 48 KB para os widgets desta UI */

/* --- Temporização --- */
#define LV_DISP_DEF_REFR_PERIOD  30
#define LV_INDEV_DEF_READ_PERIOD 30
#define LV_TICK_CUSTOM           1
#define LV_TICK_CUSTOM_INCLUDE   "Arduino.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())

/* --- Fontes usadas pela UI (ver src/ui/ui.cpp) --- */
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_MONTSERRAT_48  1
#define LV_FONT_DEFAULT        &lv_font_montserrat_14

/* Necessário para acentuação em português (á, ç, ã...) nos rótulos */
#define LV_TXT_ENC             LV_TXT_ENC_UTF8

/* --- Widgets usados --- */
#define LV_USE_LABEL   1
#define LV_USE_BTN     1
#define LV_USE_IMG     1

/* --- Recursos que não usamos (economiza flash) --- */
#define LV_USE_ANIMIMG    0
#define LV_USE_CANVAS     0
#define LV_USE_CHART      0
#define LV_USE_CALENDAR   0
#define LV_USE_KEYBOARD   0
#define LV_USE_TEXTAREA   0
#define LV_USE_TABLE      0
#define LV_USE_METER      0

/* --- Log / debug (mude para 1 se precisar diagnosticar) --- */
#define LV_USE_LOG        0
#define LV_USE_ASSERT_NULL 1

#endif /* LV_CONF_H */
