#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_rom_sys.h"          
#include "esp_timer.h"           
#include "esp_log.h"           
#include "board_init.h"

static const char *TAG = "board";
static esp_lcd_panel_handle_t s_panel = NULL;

// LVGL flush
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_draw_bitmap(s_panel, area->x1, area->y1, area->x2 + 1, area->y2 + 1, color_map);
    lv_disp_flush_ready(drv);
}

// Resistive touch (XPT2046 bit-bang SPI)
#define TOUCH_CMD_X   0xD0
#define TOUCH_CMD_Y   0x90
#define TOUCH_SAMPLES 5
#define TOUCH_X_MIN   100
#define TOUCH_X_MAX   4000
#define TOUCH_Y_MIN   100
#define TOUCH_Y_MAX   4000

static void touch_spi_init(void)
{
    gpio_config_t out = {
        .pin_bit_mask = (1ULL<<PIN_TOUCH_CS)|(1ULL<<PIN_TOUCH_CLK)|(1ULL<<PIN_TOUCH_MOSI),
        .mode = GPIO_MODE_OUTPUT, .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&out);
    gpio_config_t in = {
        .pin_bit_mask = (1ULL<<PIN_TOUCH_MISO),
        .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&in);
    gpio_config_t irq = {
        .pin_bit_mask = (1ULL<<PIN_TOUCH_IRQ),
        .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&irq);
    gpio_set_level(PIN_TOUCH_CS, 1);
    gpio_set_level(PIN_TOUCH_CLK, 0);
}

static uint16_t touch_read_axis(uint8_t cmd)
{
    uint16_t result = 0;
    gpio_set_level(PIN_TOUCH_CS, 0);
    for (int i = 7; i >= 0; i--) {
        gpio_set_level(PIN_TOUCH_MOSI, (cmd >> i) & 1);
        gpio_set_level(PIN_TOUCH_CLK, 1); esp_rom_delay_us(1);
        gpio_set_level(PIN_TOUCH_CLK, 0); esp_rom_delay_us(1);
    }
    for (int i = 11; i >= 0; i--) {
        gpio_set_level(PIN_TOUCH_CLK, 1); esp_rom_delay_us(1);
        result |= (gpio_get_level(PIN_TOUCH_MISO) << i);
        gpio_set_level(PIN_TOUCH_CLK, 0); esp_rom_delay_us(1);
    }
    gpio_set_level(PIN_TOUCH_CS, 1);
    return result;
}

static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    if (gpio_get_level(PIN_TOUCH_IRQ) != 0) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }
    uint32_t xs = 0, ys = 0;
    for (int i = 0; i < TOUCH_SAMPLES; i++) {
        xs += touch_read_axis(TOUCH_CMD_X);
        ys += touch_read_axis(TOUCH_CMD_Y);
    }
    uint16_t rx = xs / TOUCH_SAMPLES;
    uint16_t ry = ys / TOUCH_SAMPLES;

    if (rx < TOUCH_X_MIN) rx = TOUCH_X_MIN;
    if (rx > TOUCH_X_MAX) rx = TOUCH_X_MAX;
    if (ry < TOUCH_Y_MIN) ry = TOUCH_Y_MIN;
    if (ry > TOUCH_Y_MAX) ry = TOUCH_Y_MAX;

    data->point.x = (rx - TOUCH_X_MIN) * LCD_H_RES / (TOUCH_X_MAX - TOUCH_X_MIN);
    data->point.y = (ry - TOUCH_Y_MIN) * LCD_V_RES / (TOUCH_Y_MAX - TOUCH_Y_MIN);
    data->state   = LV_INDEV_STATE_PR;
}

// LVGL tick
void board_lvgl_tick(void *arg) { lv_tick_inc(5); }

void board_set_backlight(uint8_t brightness)
{
    // brightness: 0-255 (0=OFF, 255=MAX) 
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_PWM_CHANNEL, brightness);  
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_PWM_CHANNEL);         
}

static void board_init_backlight(void)
{
    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE, 
        .duty_resolution = BACKLIGHT_PWM_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = BACKLIGHT_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_cfg));

    // Configured LED channel 
    ledc_channel_config_t channel_cfg = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PIN_LCD_BL,
        .duty = 255, 
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_cfg));
    ESP_LOGI(TAG, "Backlight PWM initialized at %d Hz with %d-bit resolution", 
             BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RESOLUTION);
}

// Board init 
void board_init(void)
{
    ESP_LOGI(TAG, "Init RGB LCD 480x272 - CrowPanel 4.3\"");

    board_init_backlight();
    board_set_backlight(255);  

    // RGB Pannel 
    esp_lcd_rgb_panel_config_t panel_cfg = {
        .data_width        = 16,
        .psram_trans_align = 64,
        .num_fbs           = 1,
        .clk_src           = LCD_CLK_SRC_PLL160M,
        .disp_gpio_num     = GPIO_NUM_NC,
        .pclk_gpio_num     = PIN_LCD_PCLK,
        .vsync_gpio_num    = PIN_LCD_VSYNC,
        .hsync_gpio_num    = PIN_LCD_HSYNC,
        .de_gpio_num       = PIN_LCD_DE,
        .data_gpio_nums = {
            PIN_LCD_D0, PIN_LCD_D1, PIN_LCD_D2, PIN_LCD_D3, PIN_LCD_D4,
            PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7, PIN_LCD_D8, PIN_LCD_D9, PIN_LCD_D10,
            PIN_LCD_D11, PIN_LCD_D12, PIN_LCD_D13, PIN_LCD_D14, PIN_LCD_D15,
        },
        .timings = {
            .pclk_hz           = 8000000, 
            .h_res             = LCD_H_RES,
            .v_res             = LCD_V_RES,
            .hsync_back_porch  = 43,
            .hsync_front_porch = 8,
            .hsync_pulse_width = 4,
            .vsync_back_porch  = 12,
            .vsync_front_porch = 8,
            .vsync_pulse_width = 4,
            .flags = {
                .pclk_active_neg = LCD_PCLK_ACTIVE_NEG,  
            },
        },
        .flags = {
            .fb_in_psram = 1, 
        },
    };

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_cfg, &s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(s_panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(s_panel));

    // Touch 
    touch_spi_init();

    // LVGL init
    lv_init();

    // Draw buffer in internal DRAM 
    static lv_color_t draw_buf_data[LCD_H_RES * LVGL_DRAW_BUF_LINES];
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, draw_buf_data, NULL, LCD_H_RES * LVGL_DRAW_BUF_LINES);

    // Display Driver 
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res  = LCD_H_RES;
    disp_drv.ver_res  = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    // Touch input device 
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_cb;
    lv_indev_drv_register(&indev_drv);

    // LVGL 5ms Tick Timer 
    const esp_timer_create_args_t tick_args = { .callback = board_lvgl_tick, .name = "lvgl_tick" };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 5000));

    ESP_LOGI(TAG, "Board init done");
}