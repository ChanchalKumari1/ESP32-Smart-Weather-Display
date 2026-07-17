#pragma once

#include "defines.h"
#include "ui.h"

static const char *API_TAG = "api";

static weather_data_t s_weather_data = { .valid = false };

static double s_lat = DEFAULT_LAT;
static double s_lon = DEFAULT_LON;

static bool s_api_is_updating       = false;
static bool s_api_request_pending   = false;
static int  s_api_retry_count       = 0;
static esp_http_client_handle_t s_api_http_client = NULL;
static EventGroupHandle_t s_api_event_group = NULL;

#define API_FETCH_BIT   BIT0
#define API_MAX_RETRIES 3

static esp_err_t api_http_event_handler(esp_http_client_event_t *evt)
{
    static char *response_buffer = NULL;
    static int response_len = 0;

    switch (evt->event_id) {
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(API_TAG, "HTTP Connected to server");
            break;

        case HTTP_EVENT_ON_DATA:
            if (!response_buffer) {
                response_buffer = malloc(4096);
                if (!response_buffer) {
                    ESP_LOGE(API_TAG, "Failed to allocate response buffer");
                    return ESP_ERR_NO_MEM;
                }
                response_len = 0;
            }
            if (response_len + evt->data_len < 4096) {
                memcpy(response_buffer + response_len, evt->data, evt->data_len);
                response_len += evt->data_len;
            } else {
                ESP_LOGW(API_TAG, "Response buffer overflow");
            }
            break;

        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(API_TAG, "HTTP request completed, received %d bytes", response_len);
            if (response_buffer && response_len > 0) {
                response_buffer[response_len] = '\0';

                cJSON *root = cJSON_Parse(response_buffer);
                if (root) {
                    cJSON *cod = cJSON_GetObjectItem(root, "cod");
                    bool api_error = false;
                    if (cod) {
                        if (cJSON_IsNumber(cod) && cod->valueint != 200) {
                            ESP_LOGE(API_TAG, "API Error code: %d", cod->valueint);
                            api_error = true;
                        } else if (cJSON_IsString(cod) && strcmp(cod->valuestring, "200") != 0) {
                            ESP_LOGE(API_TAG, "API Error: %s", cod->valuestring);
                            api_error = true;
                        }
                    }

                    if (!api_error) {
                        cJSON *main_obj    = cJSON_GetObjectItem(root, "main");
                        cJSON *weather_arr = cJSON_GetObjectItem(root, "weather");
                        cJSON *name        = cJSON_GetObjectItem(root, "name");
                        cJSON *wind_obj    = cJSON_GetObjectItem(root, "wind");
                        cJSON *visibility  = cJSON_GetObjectItem(root, "visibility");

                        if (main_obj && weather_arr && cJSON_IsArray(weather_arr) &&
                            cJSON_GetArraySize(weather_arr) > 0) {

                            cJSON *first_weather = cJSON_GetArrayItem(weather_arr, 0);

                            if (name && cJSON_IsString(name) && strlen(name->valuestring) > 0) {
                                strncpy(s_weather_data.city, name->valuestring, sizeof(s_weather_data.city) - 1);
                                s_weather_data.city[sizeof(s_weather_data.city) - 1] = '\0';
                            }

                            cJSON *temp     = cJSON_GetObjectItem(main_obj, "temp");
                            cJSON *humidity = cJSON_GetObjectItem(main_obj, "humidity");
                            cJSON *pressure = cJSON_GetObjectItem(main_obj, "pressure");
                            cJSON *desc     = cJSON_GetObjectItem(first_weather, "description");
                            cJSON *icon     = cJSON_GetObjectItem(first_weather, "id");

                            if (temp && cJSON_IsNumber(temp)) {
                                s_weather_data.temperature = temp->valuedouble - 273.15;
                            }
                            if (humidity && cJSON_IsNumber(humidity)) {
                                s_weather_data.humidity = humidity->valuedouble;
                            }
                            if (pressure && cJSON_IsNumber(pressure)) {
                                s_weather_data.pressure = pressure->valueint;
                            }
                            if (desc && cJSON_IsString(desc)) {
                                strncpy(s_weather_data.description, desc->valuestring, sizeof(s_weather_data.description) - 1);
                                s_weather_data.description[sizeof(s_weather_data.description) - 1] = '\0';
                                if (strlen(s_weather_data.description) > 0) {
                                    s_weather_data.description[0] = toupper(s_weather_data.description[0]);
                                }
                            }
                            if (icon && cJSON_IsNumber(icon)) {
                                int icon_id = icon->valueint;
                                s_weather_data.weather_icon_id = icon_id / 100;
                                if (s_weather_data.weather_icon_id == 8) {
                                    s_weather_data.weather_icon_id = (icon_id == 800) ? 8 : 3;
                                }
                            }
                            if (wind_obj) {
                                cJSON *speed = cJSON_GetObjectItem(wind_obj, "speed");
                                cJSON *deg   = cJSON_GetObjectItem(wind_obj, "deg");
                                if (speed && cJSON_IsNumber(speed)) s_weather_data.wind_speed = speed->valuedouble;
                                if (deg && cJSON_IsNumber(deg))     s_weather_data.wind_deg = deg->valueint;
                            }
                            if (visibility && cJSON_IsNumber(visibility)) {
                                s_weather_data.visibility = visibility->valueint;
                            }

                            s_weather_data.valid = true;
                            s_api_retry_count = 0;
                            ESP_LOGI(API_TAG, "Weather updated for %s: %.1f°C, %.1f%% humidity, %s, wind %.1fm/s@%d°, %dhPa, %dm vis",
                                      s_weather_data.city, s_weather_data.temperature, s_weather_data.humidity,
                                      s_weather_data.description, s_weather_data.wind_speed, s_weather_data.wind_deg,
                                      s_weather_data.pressure, s_weather_data.visibility);

                            ui_weather_update();
                        } else {
                            ESP_LOGE(API_TAG, "Invalid JSON structure - missing required fields");
                        }
                    }
                    cJSON_Delete(root);
                } else {
                    ESP_LOGE(API_TAG, "Failed to parse weather JSON");
                }

                free(response_buffer);
                response_buffer = NULL;
                response_len = 0;
            }
            s_api_is_updating = false;
            if (s_api_event_group) {
                xEventGroupSetBits(s_api_event_group, API_FETCH_BIT);
            }
            break;

        case HTTP_EVENT_ERROR:
            ESP_LOGE(API_TAG, "HTTP request error");
            if (response_buffer) {
                free(response_buffer);
                response_buffer = NULL;
                response_len = 0;
            }
            s_api_is_updating = false;
            break;

        default:
            break;
    }
    return ESP_OK;
}

static void api_fetch_task(void *arg)
{
    char url[256];

    while (1) {
        if (s_api_request_pending) {
            s_api_request_pending = false;

            snprintf(url, sizeof(url), "%s?lat=%.6f&lon=%.6f&APPID=%s",
                     WEATHER_BASE_URL, s_lat, s_lon, WEATHER_API_KEY);
            ESP_LOGI(API_TAG, "Fetching weather for lat=%.6f, lon=%.6f", s_lat, s_lon);
            ESP_LOGI(API_TAG, "URL: %s", url);

            esp_http_client_config_t config = {
                .url = url,
                .method = HTTP_METHOD_GET,
                .event_handler = api_http_event_handler,
                .timeout_ms = 15000,
                .buffer_size = 4096,
                .buffer_size_tx = 2048,
                .keep_alive_enable = false,
                .disable_auto_redirect = true,
            };

            s_api_http_client = esp_http_client_init(&config);
            if (s_api_http_client) {
                s_api_is_updating = true;
                esp_err_t err = esp_http_client_perform(s_api_http_client);
                if (err != ESP_OK) {
                    ESP_LOGE(API_TAG, "HTTP request failed: %s", esp_err_to_name(err));
                    s_api_is_updating = false;

                    s_api_retry_count++;
                    if (s_api_retry_count < API_MAX_RETRIES) {
                        ESP_LOGI(API_TAG, "Retry %d/%d in 5 seconds", s_api_retry_count, API_MAX_RETRIES);
                        vTaskDelay(pdMS_TO_TICKS(5000));
                        s_api_request_pending = true;
                    } else {
                        ESP_LOGE(API_TAG, "Max retries reached, giving up");
                        s_api_retry_count = 0;
                    }
                } else {
                    s_api_retry_count = 0;
                }
                esp_http_client_cleanup(s_api_http_client);
                s_api_http_client = NULL;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void api_init(void)
{
    s_api_event_group = xEventGroupCreate();
    xTaskCreatePinnedToCore(api_fetch_task, "api_fetch", 8192, NULL, 3, NULL, 1);
    ESP_LOGI(API_TAG, "Weather API service initialized");
}

void api_set_coords(double lat, double lon)
{
    s_lat = lat;
    s_lon = lon;
    s_api_retry_count = 0;
    s_api_request_pending = true;
    ESP_LOGI(API_TAG, "Location set to coords: lat=%.6f, lon=%.6f", lat, lon);
}

void api_fetch_async(void)
{
    s_api_retry_count = 0;
    s_api_request_pending = true;
}

weather_data_t api_get_latest(void)
{
    return s_weather_data;
}

bool api_is_updating(void)
{
    return s_api_is_updating;
}