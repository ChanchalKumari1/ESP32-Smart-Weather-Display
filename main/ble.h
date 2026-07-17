#pragma once

#include "defines.h"

/* Expected JSON payloads written to the characteristics:
 *   WiFi char     : {"ssid":"MyNetwork","password":"MyPassword"}
 *   Location char : {"lat":31.3256,"lon":75.5792}
*/
static const char *BLE_TAG = "ble";
static uint8_t s_ble_own_addr_type;

/* Single Service UUID: 4fafc201-1fb5-459e-8fcc-c5c9c331914b */
static const ble_uuid128_t MAIN_SERVICE_UUID =
    BLE_UUID128_INIT(0x4b, 0x91, 0x31, 0xc3, 0xc9, 0xc5, 0xcc, 0x8f,
                      0x9e, 0x45, 0xb5, 0x1f, 0x01, 0xc2, 0xaf, 0x4f);

/* WiFi Characteristic UUID: 6e400002-b5a3-f393-e0a9-e50e24dcca9e */
static const ble_uuid128_t WIFI_CHAR_UUID =
    BLE_UUID128_INIT(0x9e, 0xca, 0xdc, 0x24, 0x0e, 0xe5, 0xa9, 0xe0,
                      0x93, 0xf3, 0xa3, 0xb5, 0x02, 0x00, 0x40, 0x6e);

/* Location Characteristic UUID: beb5483e-36e1-4688-b7f5-ea07361b26a8 */
static const ble_uuid128_t LOCATION_CHAR_UUID =
    BLE_UUID128_INIT(0xa8, 0x26, 0x1b, 0x36, 0x07, 0xea, 0xf5, 0xb7,
                      0x88, 0x46, 0xe1, 0x36, 0x3e, 0x48, 0xb5, 0xbe);

static int ble_copy_mbuf_to_buf(struct os_mbuf *om, char *out, size_t out_size)
{
    uint16_t len = OS_MBUF_PKTLEN(om);
    if (len >= out_size) {
        len = out_size - 1;
    }
    int rc = ble_hs_mbuf_to_flat(om, out, len, NULL);
    if (rc != 0) {
        return rc;
    }
    out[len] = '\0';
    return 0;
}

// Normalize curly quotes to standard JSON quotes
static void normalize_json_quotes(char *json)
{
    char *src = json;
    char *dst = json;
    
    while (*src) {
        // Check for UTF-8 curly quote sequences
        if ((unsigned char)*src == 0xE2 && 
            (unsigned char)*(src + 1) == 0x80) {
            unsigned char next2 = (unsigned char)*(src + 2);
            // Skip the entire UTF-8 sequence and replace with standard quote
            if (next2 == 0x9C || next2 == 0x9D || next2 == 0x98 || next2 == 0x99) {
                *dst++ = '"';
                src += 3;
                continue;
            }
        }
        *dst++ = *src++;
    }
    *dst = '\0';
}

static int wifi_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    char json_buf[BLE_JSON_MAX_LEN];
    if (ble_copy_mbuf_to_buf(ctxt->om, json_buf, sizeof(json_buf)) != 0) {
        ESP_LOGE(BLE_TAG, "Failed to read wifi payload");
        return BLE_ATT_ERR_UNLIKELY;
    }

    ESP_LOGI(BLE_TAG, "WiFi payload: %s", json_buf);
    normalize_json_quotes(json_buf);
    ESP_LOGI(BLE_TAG, "Normalized: %s", json_buf);

    cJSON *root = cJSON_Parse(json_buf);
    if (!root) {
        ESP_LOGE(BLE_TAG, "Invalid wifi JSON");
        return BLE_ATT_ERR_UNLIKELY;
    }

    cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    cJSON *pass = cJSON_GetObjectItem(root, "password");

    if (cJSON_IsString(ssid) && cJSON_IsString(pass)) {
        app_config_save_wifi(ssid->valuestring, pass->valuestring);  
        wifi_reconnect(ssid->valuestring, pass->valuestring);      

        ESP_LOGI(BLE_TAG, "WiFi credentials updated for SSID: %s", ssid->valuestring);
    } else {
        ESP_LOGE(BLE_TAG, "WiFi JSON missing ssid/password fields");
    }

    cJSON_Delete(root);
    return 0;
}

static int location_chr_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    char json_buf[BLE_JSON_MAX_LEN];
    if (ble_copy_mbuf_to_buf(ctxt->om, json_buf, sizeof(json_buf)) != 0) {
        ESP_LOGE(BLE_TAG, "Failed to read location payload");
        return BLE_ATT_ERR_UNLIKELY;
    }

    ESP_LOGI(BLE_TAG, "Location payload: %s", json_buf);
    normalize_json_quotes(json_buf);
    ESP_LOGI(BLE_TAG, "Normalized: %s", json_buf);

    cJSON *root = cJSON_Parse(json_buf);
    if (!root) {
        ESP_LOGE(BLE_TAG, "Invalid location JSON");
        return BLE_ATT_ERR_UNLIKELY;
    }

    cJSON *lat = cJSON_GetObjectItem(root, "lat");
    cJSON *lon = cJSON_GetObjectItem(root, "lon");

    if (cJSON_IsNumber(lat) && cJSON_IsNumber(lon)) {
        double lat_val = lat->valuedouble;
        double lon_val = lon->valuedouble;

        app_config_save_location(lat_val, lon_val);   
        api_set_coords(lat_val, lon_val);            

        ESP_LOGI(BLE_TAG, "Location updated: lat=%.6f lon=%.6f", lat_val, lon_val);
    } else {
        ESP_LOGE(BLE_TAG, "Location JSON missing lat/lon fields");
    }

    cJSON_Delete(root);
    return 0;
}

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &MAIN_SERVICE_UUID.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &WIFI_CHAR_UUID.u,
                .access_cb = wifi_chr_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = &LOCATION_CHAR_UUID.u,
                .access_cb = location_chr_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            { 0 }
        },
    },
    { 0 }
};

static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    if (ctxt->op == BLE_GATT_REGISTER_OP_SVC) {
        ESP_LOGD(BLE_TAG, "registered service");
    }
}

static int gatt_svr_init(void)
{
    ble_svc_gap_init();
    ble_svc_gatt_init();

    int rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) return rc;

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) return rc;

    return 0;
}

static void ble_advertise(void);

static int ble_gap_event_cb(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(BLE_TAG, "BLE connect event, status=%d", event->connect.status);
            if (event->connect.status != 0) {
                ble_advertise();
            }
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(BLE_TAG, "BLE disconnected, reason=%d", event->disconnect.reason);
            ble_advertise();
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ble_advertise();
            break;

        default:
            break;
    }
    return 0;
}

static void ble_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;

    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)BLE_DEVICE_NAME;
    fields.name_len = strlen(BLE_DEVICE_NAME);
    fields.name_is_complete = 1;

    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(BLE_TAG, "ble_gap_adv_set_fields failed: %d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(s_ble_own_addr_type, NULL, BLE_HS_FOREVER,
                            &adv_params, ble_gap_event_cb, NULL);
    if (rc != 0) {
        ESP_LOGE(BLE_TAG, "ble_gap_adv_start failed: %d", rc);
    }
}

static void ble_on_sync(void)
{
    int rc = ble_hs_id_infer_auto(0, &s_ble_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(BLE_TAG, "ble_hs_id_infer_auto failed: %d", rc);
        return;
    }
    ble_advertise();
}

static void ble_on_reset(int reason)
{
    ESP_LOGW(BLE_TAG, "BLE host reset, reason=%d", reason);
}

static void ble_host_task(void *param)
{
    ESP_LOGI(BLE_TAG, "NimBLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void ble_init(void)
{
    esp_err_t ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(BLE_TAG, "nimble_port_init failed: %d", ret);
        return;
    }

    ble_hs_cfg.reset_cb = ble_on_reset;
    ble_hs_cfg.sync_cb = ble_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    if (gatt_svr_init() != 0) {
        ESP_LOGE(BLE_TAG, "gatt_svr_init failed");
        return;
    }

    ble_svc_gap_device_name_set(BLE_DEVICE_NAME);

    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(BLE_TAG, "BLE initialized, advertising as \"%s\"", BLE_DEVICE_NAME);
    ESP_LOGI(BLE_TAG, "  Service UUID:      %s", BLE_MAIN_SERVICE_UUID_STR);
    ESP_LOGI(BLE_TAG, "  WiFi Char UUID:    %s", BLE_WIFI_CHAR_UUID_STR);
    ESP_LOGI(BLE_TAG, "  Location Char UUID:%s", BLE_LOCATION_CHAR_UUID_STR);
}