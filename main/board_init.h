#pragma once

#include "lvgl.h"

/* ── Display resolution ── */
#define LCD_H_RES   480
#define LCD_V_RES   272

/* ── Backlight ── */
#define PIN_LCD_BL      2
#define BACKLIGHT_PWM_CHANNEL   0
#define BACKLIGHT_PWM_RESOLUTION 8     // 8-bit resolution (0-255)
#define BACKLIGHT_PWM_FREQ       5000  

/* ── RGB sync & clock ── */
#define PIN_LCD_DE      40   
#define PIN_LCD_VSYNC   41
#define PIN_LCD_HSYNC   39
#define PIN_LCD_PCLK    42

/* ── RGB565 data bus  ── */
/* Blue  B0-B4 */
#define PIN_LCD_D0      8
#define PIN_LCD_D1      3
#define PIN_LCD_D2      46
#define PIN_LCD_D3      9
#define PIN_LCD_D4      1
/* Green G0-G5 */
#define PIN_LCD_D5      5
#define PIN_LCD_D6      6
#define PIN_LCD_D7      7
#define PIN_LCD_D8      15
#define PIN_LCD_D9      16
#define PIN_LCD_D10     4
/* Red   R0-R4 */
#define PIN_LCD_D11     45
#define PIN_LCD_D12     48
#define PIN_LCD_D13     47
#define PIN_LCD_D14     21
#define PIN_LCD_D15     14

/* ── Resistive touch – XPT2046 via SPI (shares SD card SPI bus) ── */
#define PIN_TOUCH_CS    0
#define PIN_TOUCH_IRQ   36
#define PIN_TOUCH_CLK   12
#define PIN_TOUCH_MOSI  11
#define PIN_TOUCH_MISO  13

/* ── Pixel clock polarity (NV3047 needs inverted PCLK) ── */
#define LCD_PCLK_ACTIVE_NEG  1   /* pclk_active_neg = 1 */

/* ── LVGL draw buffer lines (use internal RAM – safe without PSRAM) ── */
#define LVGL_DRAW_BUF_LINES   27    /* 480*27*2 = ~26 KB, fits in DRAM */

/* ── Function prototypes ── */
void board_init(void);
void board_lvgl_tick(void *arg);
void board_set_backlight(uint8_t brightness); 