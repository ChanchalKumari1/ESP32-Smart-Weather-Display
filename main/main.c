#include "defines.h"
#include "wifi.h"   
#include "api.h"      
#include "ble.h"  
#include "ui.h"
#include "board_init.h"

static const char *TAG = "main";

app_config_t g_app_config = { 0 };

void app_config_load(void)
{
    g_app_config.lat = DEFAULT_LAT;
    g_app_config.lon = DEFAULT_LON;
    g_app_config.has_location = false;
    strncpy(g_app_config.ssid, WIFI_DEFAULT_SSID, sizeof(g_app_config.ssid) - 1);
    strncpy(g_app_config.password, WIFI_DEFAULT_PASSWORD, sizeof(g_app_config.password) - 1);
    g_app_config.has_wifi_creds = false;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No stored config found (%s), using defaults", esp_err_to_name(err));
        return;
    }

    uint8_t has_loc = 0, has_wifi = 0;
    nvs_get_u8(h, NVS_KEY_HAS_LOC, &has_loc);
    nvs_get_u8(h, NVS_KEY_HAS_WIFI, &has_wifi);

    if (has_loc) {
        size_t sz;
        double lat = 0, lon = 0;
        sz = sizeof(lat);
        if (nvs_get_blob(h, NVS_KEY_LAT, &lat, &sz) == ESP_OK) g_app_config.lat = lat;
        sz = sizeof(lon);
        if (nvs_get_blob(h, NVS_KEY_LON, &lon, &sz) == ESP_OK) g_app_config.lon = lon;
        g_app_config.has_location = true;
        ESP_LOGI(TAG, "Loaded stored location: lat=%.6f lon=%.6f", g_app_config.lat, g_app_config.lon);
    }

    if (has_wifi) {
        size_t sz;
        char ssid_buf[WIFI_MAX_SSID_LEN + 1] = {0};
        char pass_buf[WIFI_MAX_PASS_LEN + 1] = {0};
        sz = sizeof(ssid_buf);
        if (nvs_get_str(h, NVS_KEY_SSID, ssid_buf, &sz) == ESP_OK) {
            strncpy(g_app_config.ssid, ssid_buf, sizeof(g_app_config.ssid) - 1);
        }
        sz = sizeof(pass_buf);
        if (nvs_get_str(h, NVS_KEY_PASS, pass_buf, &sz) == ESP_OK) {
            strncpy(g_app_config.password, pass_buf, sizeof(g_app_config.password) - 1);
        }
        g_app_config.has_wifi_creds = true;
        ESP_LOGI(TAG, "Loaded stored WiFi credentials for SSID: %s", g_app_config.ssid);
    }

    nvs_close(h);
}

void app_config_save_location(double lat, double lon)
{
    g_app_config.lat = lat;
    g_app_config.lon = lon;
    g_app_config.has_location = true;

    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS to save location");
        return;
    }
    nvs_set_blob(h, NVS_KEY_LAT, &lat, sizeof(lat));
    nvs_set_blob(h, NVS_KEY_LON, &lon, sizeof(lon));
    nvs_set_u8(h, NVS_KEY_HAS_LOC, 1);
    nvs_commit(h);
    nvs_close(h);
}

void app_config_save_wifi(const char *ssid, const char *password)
{
    strncpy(g_app_config.ssid, ssid, sizeof(g_app_config.ssid) - 1);
    g_app_config.ssid[sizeof(g_app_config.ssid) - 1] = '\0';
    strncpy(g_app_config.password, password, sizeof(g_app_config.password) - 1);
    g_app_config.password[sizeof(g_app_config.password) - 1] = '\0';
    g_app_config.has_wifi_creds = true;

    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS to save wifi creds");
        return;
    }
    nvs_set_str(h, NVS_KEY_SSID, g_app_config.ssid);
    nvs_set_str(h, NVS_KEY_PASS, g_app_config.password);
    nvs_set_u8(h, NVS_KEY_HAS_WIFI, 1);
    nvs_commit(h);
    nvs_close(h);
}

static void lvgl_task(void *arg)
{
    ESP_LOGI(TAG, "LVGL task started");
    while (1) {
        uint32_t delay_ms = lv_timer_handler();
        if (delay_ms > 10) delay_ms = 10;
        vTaskDelay(pdMS_TO_TICKS(delay_ms ? delay_ms : 1));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Weather Station Starting...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    app_config_load();  

    wifi_init();
    board_init();
    api_init();
    ui_init();

    api_set_coords(g_app_config.lat, g_app_config.lon);

    ble_init();   

    xTaskCreatePinnedToCore(lvgl_task, "lvgl", 8192, NULL, 5, NULL, 1);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        ESP_LOGI(TAG, "Weather station running (BLE ready for location/WiFi updates)...");
    }
}