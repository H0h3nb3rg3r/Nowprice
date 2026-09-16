#include "ui.h"

// ---- Paleta ----
static lv_color_t COLOR_BG        ;
static lv_color_t COLOR_PRICE     ;
static lv_color_t COLOR_PRICE_PROMO;
static lv_color_t COLOR_TEXT_MUTED;
static lv_color_t COLOR_ACCENT    ;

// ---- Widgets ----
static lv_obj_t *scr;
static lv_obj_t *lbl_name;
static lv_obj_t *lbl_price;
static lv_obj_t *lbl_secondary;
static lv_obj_t *badge_promo;
static lv_obj_t *btn_audio;
static lv_obj_t *lbl_mesh_dot;
static lv_obj_t *lbl_provisioning;

static AudioButtonCallback g_audioCb = nullptr;

static void audio_btn_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        if (g_audioCb) g_audioCb();
    }
}

// Formata centavos como "R$ 12,90"
static void format_price(uint32_t cents, char *out, size_t outLen) {
    uint32_t reais = cents / 100;
    uint32_t centavos = cents % 100;
    snprintf(out, outLen, "R$ %lu,%02lu", (unsigned long)reais, (unsigned long)centavos);
}

void ui_init() {
    COLOR_BG          = lv_color_hex(0xFFFFFF);
    COLOR_PRICE        = lv_color_hex(0xC81E1E); // vermelho forte, destaque
    COLOR_PRICE_PROMO   = lv_color_hex(0x1E8E3E); // verde promoção
    COLOR_TEXT_MUTED   = lv_color_hex(0x555555);
    COLOR_ACCENT       = lv_color_hex(0x1565C0);

    scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, COLOR_BG, 0);

    // Nome do produto (topo)
    lbl_name = lv_label_create(scr);
    lv_obj_set_width(lbl_name, 220);
    lv_label_set_long_mode(lbl_name, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(lbl_name, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_name, LV_ALIGN_TOP_MID, 0, 12);
    lv_label_set_text(lbl_name, "Carregando produto...");

    // Selo de promoção (some/aparece conforme o estado)
    badge_promo = lv_label_create(scr);
    lv_obj_set_style_bg_color(badge_promo, COLOR_PRICE_PROMO, 0);
    lv_obj_set_style_bg_opa(badge_promo, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(badge_promo, lv_color_white(), 0);
    lv_obj_set_style_pad_all(badge_promo, 4, 0);
    lv_obj_set_style_radius(badge_promo, 6, 0);
    lv_label_set_text(badge_promo, "PROMOÇÃO");
    lv_obj_align(badge_promo, LV_ALIGN_TOP_MID, 0, 46);
    lv_obj_add_flag(badge_promo, LV_OBJ_FLAG_HIDDEN);

    // Preço principal (bem grande, centralizado)
    lbl_price = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_price, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lbl_price, COLOR_PRICE, 0);
    lv_obj_align(lbl_price, LV_ALIGN_CENTER, 0, -20);
    lv_label_set_text(lbl_price, "R$ 0,00");

    // Informação secundária: unidade / código
    lbl_secondary = lv_label_create(scr);
    lv_obj_set_style_text_font(lbl_secondary, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lbl_secondary, COLOR_TEXT_MUTED, 0);
    lv_obj_align(lbl_secondary, LV_ALIGN_CENTER, 0, 40);
    lv_label_set_text(lbl_secondary, "");

    // Botão de acessibilidade / áudio do preço
    btn_audio = lv_btn_create(scr);
    lv_obj_set_size(btn_audio, 220, 50);
    lv_obj_align(btn_audio, LV_ALIGN_BOTTOM_MID, 0, -14);
    lv_obj_set_style_bg_color(btn_audio, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(btn_audio, 10, 0);
    lv_obj_add_event_cb(btn_audio, audio_btn_event_cb, LV_EVENT_CLICKED, nullptr);

    lv_obj_t *btn_label = lv_label_create(btn_audio);
    lv_label_set_text(btn_label, LV_SYMBOL_AUDIO "  Ouvir preço");
    lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_20, 0);
    lv_obj_center(btn_label);

    // Indicador discreto de atividade da mesh (cantinho superior direito)
    lbl_mesh_dot = lv_label_create(scr);
    lv_label_set_text(lbl_mesh_dot, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(lbl_mesh_dot, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(lbl_mesh_dot, LV_ALIGN_TOP_RIGHT, -6, 6);

    // Indicador de modo provisionamento
    lbl_provisioning = lv_label_create(scr);
    lv_label_set_text(lbl_provisioning, "MODO CONFIG");
    lv_obj_set_style_text_color(lbl_provisioning, lv_color_hex(0xB8860B), 0);
    lv_obj_align(lbl_provisioning, LV_ALIGN_TOP_LEFT, 6, 6);
    lv_obj_add_flag(lbl_provisioning, LV_OBJ_FLAG_HIDDEN);
}

void ui_update_product(const ProductInfo &info) {
    lv_label_set_text(lbl_name, info.name);

    char priceStr[16];
    format_price(info.priceCents, priceStr, sizeof(priceStr));
    lv_label_set_text(lbl_price, priceStr);
    lv_obj_set_style_text_color(lbl_price, info.promo ? COLOR_PRICE_PROMO : COLOR_PRICE, 0);

    if (info.promo) {
        lv_obj_clear_flag(badge_promo, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(badge_promo, LV_OBJ_FLAG_HIDDEN);
    }

    char secondary[48];
    if (strlen(info.code) > 0) {
        snprintf(secondary, sizeof(secondary), "%s | Cód: %s", info.unit, info.code);
    } else {
        snprintf(secondary, sizeof(secondary), "%s", info.unit);
    }
    lv_label_set_text(lbl_secondary, secondary);
}

void ui_set_audio_button_callback(AudioButtonCallback cb) {
    g_audioCb = cb;
}

void ui_pulse_mesh_indicator() {
    lv_obj_set_style_text_color(lbl_mesh_dot, COLOR_ACCENT, 0);
    // Um timer simples volta a cor ao normal após 400ms.
    lv_timer_t *t = lv_timer_create([](lv_timer_t *timer) {
        lv_obj_set_style_text_color(lbl_mesh_dot, lv_color_hex(0xCCCCCC), 0);
        lv_timer_del(timer);
    }, 400, nullptr);
    LV_UNUSED(t);
}

void ui_set_provisioning_indicator(bool active) {
    if (active) lv_obj_clear_flag(lbl_provisioning, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(lbl_provisioning, LV_OBJ_FLAG_HIDDEN);
}
