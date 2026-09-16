// ============================================================
// User_Setup.h - Configuração TFT_eSPI para ESP32 "Cheap Yellow
// Display" (CYD) 2.8" 240x320, driver ILI9341, touch resistivo XPT2046
//
// ATENÇÃO: existem várias revisões da placa CYD com pinagens
// ligeiramente diferentes. Confira a serigrafia da sua placa e
// ajuste os pinos abaixo se necessário (essa é a pinagem mais comum,
// modelo com conector USB-C e sem CN1 separado).
// ============================================================

#define ILI9341_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// --- Barramento SPI do display ---
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15   // Chip select do display
#define TFT_DC    2   // Data/Command
#define TFT_RST  -1   // Reset do display ligado ao EN da placa -> deixar -1

// --- Backlight ---
#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

// --- Touch resistivo XPT2046 (barramento SPI compartilhado) ---
#define TOUCH_CS 33
// Muitas CYD usam um 2º barramento SPI (VSPI) para o touch;
// se o toque não funcionar, troque para SPI dedicado - ver README.

// --- Fontes carregadas (mantenha só o necessário para economizar flash) ---
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// --- Velocidade do barramento SPI ---
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
#define SPI_TOUCH_FREQUENCY 2500000
