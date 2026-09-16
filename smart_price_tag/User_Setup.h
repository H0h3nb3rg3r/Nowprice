#ifndef USER_SETUP_H
#define USER_SETUP_H

// ESP32 CYD (Cheap Yellow Display) com ST7789
#define ST7789_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 320

// Pinos para ESP32 CYD
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4
#define TFT_BL   16

// SPI Pins
#define TFT_MISO 19
#define TFT_MOSI 23
#define TFT_SCLK 18

// Touch (XPT2046)
#define TOUCH_CS 21

// Configuração de backlight
#define TFT_BACKLIGHT_ON HIGH

// Configuração SPI
#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  16000000
#define SPI_TOUCH_FREQUENCY  2500000

#endif