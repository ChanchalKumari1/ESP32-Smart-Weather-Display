#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <ctype.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "lvgl.h" 

// WiFi
#define WIFI_DEFAULT_SSID       "XXXXXXXXX"
#define WIFI_DEFAULT_PASSWORD   "XXXXXXXXX"
#define WIFI_MAX_SSID_LEN       32
#define WIFI_MAX_PASS_LEN       64

// location 
#define DEFAULT_LAT   31.3256
#define DEFAULT_LON   75.5792

// NVS 
#define NVS_NAMESPACE       "app_cfg"
#define NVS_KEY_SSID        "wifi_ssid"
#define NVS_KEY_PASS        "wifi_pass"
#define NVS_KEY_LAT         "loc_lat"
#define NVS_KEY_LON         "loc_lon"
#define NVS_KEY_HAS_LOC     "has_loc"
#define NVS_KEY_HAS_WIFI    "has_wifi"

// BLE
#define BLE_DEVICE_NAME     "WeatherStation"
#define BLE_MAIN_SERVICE_UUID_STR   "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BLE_LOCATION_CHAR_UUID_STR  "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BLE_WIFI_CHAR_UUID_STR      "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define BLE_JSON_MAX_LEN   256

// API 
#define WEATHER_API_KEY    "XXXXXXXXXXXXXXXXXX"
#define WEATHER_BASE_URL   "http://api.openweathermap.org/data/2.5/weather"

// data structs
typedef struct {
    char  city[64];         
    float temperature;     
    float humidity;         
    char  description[64];
    int   weather_icon_id;
    float wind_speed;        
    int   wind_deg;         
    int   pressure;       
    int   visibility;      
    bool  valid;
} weather_data_t;

/* Persisted / runtime app configuration (location + WiFi creds) */
typedef struct {
    double lat;
    double lon;
    bool   has_location;                     
    char   ssid[WIFI_MAX_SSID_LEN + 1];
    char   password[WIFI_MAX_PASS_LEN + 1];
    bool   has_wifi_creds;                    
} app_config_t;

extern app_config_t g_app_config;   


// function prototypes

/* -- main.c: NVS-backed config persistence -- */
void app_config_load(void);
void app_config_save_location(double lat, double lon);
void app_config_save_wifi(const char *ssid, const char *password);

/* -- wifi.h -- */
void wifi_init(void);
void wifi_reconnect(const char *ssid, const char *password);

/* -- api.h -- */
void api_init(void);
void api_set_coords(double lat, double lon);
void api_fetch_async(void);
weather_data_t api_get_latest(void);
bool api_is_updating(void);

/* -- ble.h -- */
void ble_init(void);
