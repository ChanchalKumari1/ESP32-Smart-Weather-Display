#pragma once

#include "defines.h"

static const char *WIFI_TAG = "wifi";
static bool s_wifi_stack_started = false;

static void wifi_on_got_ip(void)
{
    ip_addr_t dns_server;
    IP_ADDR4(&dns_server, 8, 8, 8, 8);
    dns_setserver(0, &dns_server);
    IP_ADDR4(&dns_server, 1, 1, 1, 1);
    dns_setserver(1, &dns_server);
    ESP_LOGI(WIFI_TAG, "DNS servers configured: 8.8.8.8, 1.1.1.1");

    struct hostent *host = gethostbyname("api.openweathermap.org");
    if (!host) {
        ESP_LOGE(WIFI_TAG, "DNS resolution failed");
        return;
    }
    ESP_LOGI(WIFI_TAG, "DNS resolved successfully!");

    api_set_coords(g_app_config.lat, g_app_config.lon);
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(WIFI_TAG, "WiFi started, connecting...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(WIFI_TAG, "WiFi connected to AP");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(WIFI_TAG, "WiFi disconnected, trying to reconnect...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(WIFI_TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_on_got_ip();
    }
}

void wifi_init(void)
{
    ESP_LOGI(WIFI_TAG, "Initializing WiFi...");

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                         &wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                         &wifi_event_handler, NULL, &instance_got_ip);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = { 0 };
    strncpy((char *)wifi_config.sta.ssid, g_app_config.ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, g_app_config.password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    s_wifi_stack_started = true;
    ESP_LOGI(WIFI_TAG, "WiFi initialization complete (SSID: %s)", g_app_config.ssid);
}

void wifi_reconnect(const char *ssid, const char *password)
{
    if (!s_wifi_stack_started) {
        ESP_LOGW(WIFI_TAG, "wifi_reconnect called before wifi_init; ignoring");
        return;
    }

    ESP_LOGI(WIFI_TAG, "Reconnecting WiFi with new credentials, SSID: %s", ssid);

    esp_wifi_disconnect();

    wifi_config_t wifi_config = { 0 };
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_connect();
}