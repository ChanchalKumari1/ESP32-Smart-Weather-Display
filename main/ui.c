#include "defines.h"

static const char *TAG = "ui";

/* ── Modern Weather Theme Colors ── */
#define C_BG lv_color_hex(0xF0F4F8)              // Light gray-blue background
#define C_CARD_BG lv_color_hex(0xFFFFFF)          // Pure white card
#define C_TEXT_PRIMARY lv_color_hex(0x1A2332)     // Dark blue-black text
#define C_TEXT_SECONDARY lv_color_hex(0x5A6C7E)   // Gray text
#define C_TEXT_LIGHT lv_color_hex(0x8A9BB0)       // Light gray text
#define C_TEMP_HOT lv_color_hex(0xFF6B35)         // Orange for hot
#define C_TEMP_COLD lv_color_hex(0x2196F3)        // Blue for cold
#define C_ACCENT_BLUE lv_color_hex(0x4A90D9)      // Accent blue
#define C_DIVIDER lv_color_hex(0xE8ECF1)          // Divider line
#define C_CLOUD_LIGHT lv_color_hex(0xCFD8DC)      // Light gray cloud
#define C_CLOUD_MID lv_color_hex(0xB0BEC5)        // Mid gray cloud
#define C_CLOUD_DARK lv_color_hex(0x78909C)       // Dark gray cloud (rain/storm)
#define C_RAIN_BLUE lv_color_hex(0x4FC3F7)        // Raindrop blue
#define C_BOLT_YELLOW lv_color_hex(0xFFC107)      // Lightning bolt

/* Point buffers for lv_line_set_points() must stay alive for the life of the line object,
 * so they are file-scope statics, re-filled each time an icon is (re)drawn. */
static lv_point_t s_rain_pts[5][2];
static lv_point_t s_bolt_pts[5];
static lv_point_t s_mist_pts[4][2];

static void icon_add_circle(lv_obj_t *parent, lv_coord_t x, lv_coord_t y, lv_coord_t d, lv_color_t color, lv_opa_t opa)
{
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_size(c, d, d);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_style_radius(c, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(c, color, 0);
    lv_obj_set_style_bg_opa(c, opa, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_pad_all(c, 0, 0);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(c, LV_OBJ_FLAG_CLICKABLE);
}

static lv_obj_t *icon_add_line(lv_obj_t *parent, lv_point_t *points, uint16_t cnt, lv_coord_t width, lv_color_t color)
{
    lv_obj_t *l = lv_line_create(parent);
    lv_line_set_points(l, points, cnt);
    lv_obj_set_style_line_width(l, width, 0);
    lv_obj_set_style_line_color(l, color, 0);
    lv_obj_set_style_line_rounded(l, true, 0);
    lv_obj_clear_flag(l, LV_OBJ_FLAG_CLICKABLE);
    return l;
}

/* Cloud made of three overlapping "puff" circles + a rounded base */
static void icon_draw_cloud(lv_obj_t *parent, lv_coord_t ox, lv_coord_t oy, lv_color_t color)
{
    lv_obj_t *base = lv_obj_create(parent);
    lv_obj_set_size(base, 34, 16);
    lv_obj_set_pos(base, ox, oy + 10);
    lv_obj_set_style_radius(base, 8, 0);
    lv_obj_set_style_bg_color(base, color, 0);
    lv_obj_set_style_border_width(base, 0, 0);
    lv_obj_set_style_pad_all(base, 0, 0);
    lv_obj_clear_flag(base, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(base, LV_OBJ_FLAG_CLICKABLE);

    icon_add_circle(parent, ox + 2,  oy + 2, 16, color, LV_OPA_COVER);
    icon_add_circle(parent, ox + 12, oy - 4, 20, color, LV_OPA_COVER);
    icon_add_circle(parent, ox + 24, oy + 2, 14, color, LV_OPA_COVER);
}

/* Draws the graphic for a given OpenWeatherMap icon-group id (1-9,10,11,13,50 as bucketed by api.h) */
static void draw_weather_icon_graphic(lv_obj_t *parent, int icon_id)
{
    lv_obj_clean(parent);

    switch (icon_id) {
        case 8: // Clear sky
            icon_add_circle(parent, 12, 4, 32, C_TEMP_HOT, LV_OPA_30);   // glow
            icon_add_circle(parent, 18, 10, 20, C_TEMP_HOT, LV_OPA_COVER);
            break;

        case 2: // Few clouds
        case 3: // Scattered clouds
            icon_draw_cloud(parent, 8, 8, C_CLOUD_MID);
            break;

        case 4: // Broken clouds
            icon_draw_cloud(parent, 4, 4, C_CLOUD_DARK);
            icon_draw_cloud(parent, 18, 12, C_CLOUD_LIGHT);
            break;

        case 5:  // Rain
        case 9:  // Heavy rain
        {
            icon_draw_cloud(parent, 8, 2, C_CLOUD_DARK);
            int drops = (icon_id == 9) ? 5 : 3;
            int start_x = (icon_id == 9) ? 10 : 16;
            for (int i = 0; i < drops; i++) {
                lv_coord_t x = start_x + i * 8;
                s_rain_pts[i][0].x = x;      s_rain_pts[i][0].y = 26;
                s_rain_pts[i][1].x = x - 4;  s_rain_pts[i][1].y = 38;
                icon_add_line(parent, s_rain_pts[i], 2, 3, C_RAIN_BLUE);
            }
            break;
        }

        case 10: // Thunderstorm
        case 11:
            icon_draw_cloud(parent, 8, 0, C_CLOUD_DARK);
            s_bolt_pts[0].x = 26; s_bolt_pts[0].y = 20;
            s_bolt_pts[1].x = 18; s_bolt_pts[1].y = 30;
            s_bolt_pts[2].x = 26; s_bolt_pts[2].y = 30;
            s_bolt_pts[3].x = 20; s_bolt_pts[3].y = 40;
            icon_add_line(parent, s_bolt_pts, 4, 3, C_BOLT_YELLOW);
            break;

        case 13: // Snow
            icon_draw_cloud(parent, 8, 2, C_CLOUD_LIGHT);
            icon_add_circle(parent, 14, 28, 6, C_ACCENT_BLUE, LV_OPA_COVER);
            icon_add_circle(parent, 26, 32, 6, C_ACCENT_BLUE, LV_OPA_COVER);
            icon_add_circle(parent, 38, 26, 6, C_ACCENT_BLUE, LV_OPA_COVER);
            break;

        case 50: // Mist
            for (int i = 0; i < 4; i++) {
                lv_coord_t y = 8 + i * 8;
                lv_coord_t inset = (i % 2) ? 10 : 0;
                s_mist_pts[i][0].x = 2 + inset;  s_mist_pts[i][0].y = y;
                s_mist_pts[i][1].x = 50 - inset; s_mist_pts[i][1].y = y;
                icon_add_line(parent, s_mist_pts[i], 2, 3, C_TEXT_LIGHT);
            }
            break;

        default: // Unknown / not yet fetched
            icon_draw_cloud(parent, 8, 8, C_CLOUD_LIGHT);
            break;
    }
}

/* ── Wind direction (compass) from degrees ── */
static const char *get_wind_compass(int deg)
{
    static const char *dirs[] = {
        "N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
        "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"
    };
    int idx = ((deg % 360) + 360) % 360;
    idx = (idx + 11) / 22; // 16 sectors of 22.5°, rounded
    return dirs[idx % 16];
}

/* ── UI Elements ── */
static lv_obj_t *s_location_label = NULL;
static lv_obj_t *s_temp_label = NULL;
static lv_obj_t *s_weather_icon_container = NULL;
static lv_obj_t *s_description_label = NULL;
static lv_obj_t *s_feels_like_label = NULL;
static lv_obj_t *s_humidity_label = NULL;
static lv_obj_t *s_wind_label = NULL;
static lv_obj_t *s_visibility_label = NULL;
static lv_obj_t *s_pressure_label = NULL;
static lv_obj_t *s_loading_label = NULL;
static lv_obj_t *s_temp_unit_label = NULL;

/* ── UI Update Function ── */
void ui_weather_update(void)
{
    weather_data_t data = api_get_latest();

    if (!data.valid) {
        if (s_loading_label) {
            lv_label_set_text(s_loading_label, "Fetching weather...");
            lv_obj_clear_flag(s_loading_label, LV_OBJ_FLAG_HIDDEN);
        }
        return;
    }

    // Hide loading
    if (s_loading_label) {
        lv_obj_add_flag(s_loading_label, LV_OBJ_FLAG_HIDDEN);
    }

    // Update location
    if (s_location_label) {
        lv_label_set_text(s_location_label, data.city);
    }

    // Update temperature with °C
    if (s_temp_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f", data.temperature);
        lv_label_set_text(s_temp_label, buf);
        
        // Color based on temperature
        if (data.temperature > 30) {
            lv_obj_set_style_text_color(s_temp_label, C_TEMP_HOT, 0);
        } else if (data.temperature < 15) {
            lv_obj_set_style_text_color(s_temp_label, C_TEMP_COLD, 0);
        } else {
            lv_obj_set_style_text_color(s_temp_label, C_TEXT_PRIMARY, 0);
        }
    }

    // Update temperature unit °C
    if (s_temp_unit_label) {
        lv_label_set_text(s_temp_unit_label, "°C");
    }

    // Update weather icon (graphic only — description text already carries the words)
    if (s_weather_icon_container) {
        draw_weather_icon_graphic(s_weather_icon_container, data.weather_icon_id);
    }

    // Update description
    if (s_description_label) {
        char desc[64];
        strncpy(desc, data.description, sizeof(desc) - 1);
        desc[sizeof(desc) - 1] = '\0';
        if (strlen(desc) > 0) {
            desc[0] = toupper(desc[0]);
        }
        lv_label_set_text(s_description_label, desc);
    }

    // Update feels like
    if (s_feels_like_label) {
        char buf[32];
        snprintf(buf, sizeof(buf), "Feels like %.0f°C", data.temperature);
        lv_label_set_text(s_feels_like_label, buf);
    }

    // Update humidity
    if (s_humidity_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f%%", data.humidity);
        lv_label_set_text(s_humidity_label, buf);
    }

    // Update wind from API data
    if (s_wind_label) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%.1f m/s %s", data.wind_speed, get_wind_compass(data.wind_deg));
        lv_label_set_text(s_wind_label, buf);
    }

    // Update visibility from API data (meters -> km)
    if (s_visibility_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f km", data.visibility / 1000.0f);
        lv_label_set_text(s_visibility_label, buf);
    }

    // Update pressure from API data
    if (s_pressure_label) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d hPa", data.pressure);
        lv_label_set_text(s_pressure_label, buf);
    }

    ESP_LOGI(TAG, "UI updated with weather data");
}

/* ── UI Initialization ── */
void ui_init(void)
{
    lv_obj_t *scr = lv_scr_act();

    // Clean white background
    lv_obj_set_style_bg_color(scr, C_BG, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Main Card ── */
    lv_obj_t *card = lv_obj_create(scr);
    lv_obj_set_size(card, 460, 250);
    lv_obj_center(card);
    lv_obj_set_style_bg_color(card, C_CARD_BG, 0);
    lv_obj_set_style_radius(card, 20, 0);
    lv_obj_set_style_border_width(card, 0, 0);
    lv_obj_set_style_shadow_width(card, 20, 0);
    lv_obj_set_style_shadow_color(card, lv_color_hex(0xE0E5EC), 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Top Row: Location ── */
    lv_obj_t *top_row = lv_obj_create(card);
    lv_obj_set_size(top_row, 460, 35);
    lv_obj_set_pos(top_row, 0, 0);
    lv_obj_set_style_bg_color(top_row, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(top_row, 0, 0);
    lv_obj_set_style_radius(top_row, 0, 0);
    lv_obj_set_style_pad_all(top_row, 0, 0);
    lv_obj_clear_flag(top_row, LV_OBJ_FLAG_SCROLLABLE);

    // Location
    s_location_label = lv_label_create(top_row);
    lv_obj_align(s_location_label, LV_ALIGN_LEFT_MID, 20, 0);
    lv_obj_set_style_text_color(s_location_label, C_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(s_location_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(s_location_label, "Loading...");

    /* ── Main Weather Section ── */
    lv_obj_t *main_row = lv_obj_create(card);
    lv_obj_set_size(main_row, 460, 120);
    lv_obj_set_pos(main_row, 0, 35);
    lv_obj_set_style_bg_color(main_row, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(main_row, 0, 0);
    lv_obj_set_style_pad_all(main_row, 0, 0);
    lv_obj_clear_flag(main_row, LV_OBJ_FLAG_SCROLLABLE);

    /* ── LEFT SIDE: Large Temperature Group ── */
    lv_obj_t *temp_container = lv_obj_create(main_row);
    lv_obj_set_size(temp_container, 200, 60);
    lv_obj_align(temp_container, LV_ALIGN_LEFT_MID, 20, -10);
    lv_obj_set_style_bg_opa(temp_container, LV_OPA_TRANSP, 0); 
    lv_obj_set_style_border_width(temp_container, 0, 0);
    lv_obj_set_style_pad_all(temp_container, 0, 0);
    
    // Decrease horizontal spacing between flex items here:
    lv_obj_set_style_pad_column(temp_container, 2, 0); // Changed to 2 pixels gap
    
    lv_obj_set_flex_flow(temp_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(temp_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_clear_flag(temp_container, LV_OBJ_FLAG_SCROLLABLE);

    // Temperature number
    s_temp_label = lv_label_create(temp_container);
    lv_obj_set_style_text_color(s_temp_label, C_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(s_temp_label, &lv_font_montserrat_48, 0);  
    lv_label_set_text(s_temp_label, "--");
    
    // Push the next item even closer by stripping right-side padding
    lv_obj_set_style_pad_right(s_temp_label, 0, 0); 

    // Temperature unit °C
    s_temp_unit_label = lv_label_create(temp_container);
    lv_obj_set_style_text_color(s_temp_unit_label, C_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(s_temp_unit_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_pad_top(s_temp_unit_label, 5, 0); 
    lv_label_set_text(s_temp_unit_label, "°C");

    // Feels like below temperature (Aligned relative to the main container now)
    s_feels_like_label = lv_label_create(main_row);
    lv_obj_align(s_feels_like_label, LV_ALIGN_LEFT_MID, 20, 35);
    lv_obj_set_style_text_color(s_feels_like_label, C_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(s_feels_like_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_feels_like_label, "Feels like --°C");

    /* ── RIGHT SIDE: Weather Icon and Description ── */
    s_weather_icon_container = lv_obj_create(main_row);
    lv_obj_set_size(s_weather_icon_container, 56, 40);
    lv_obj_align(s_weather_icon_container, LV_ALIGN_RIGHT_MID, -30, -10);
    lv_obj_set_style_bg_opa(s_weather_icon_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(s_weather_icon_container, 0, 0);
    lv_obj_set_style_pad_all(s_weather_icon_container, 0, 0);
    lv_obj_clear_flag(s_weather_icon_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(s_weather_icon_container, LV_OBJ_FLAG_CLICKABLE);
    draw_weather_icon_graphic(s_weather_icon_container, 0); // placeholder cloud until first fetch

    // Description (e.g. "Clear sky")
    s_description_label = lv_label_create(main_row);
    lv_obj_align(s_description_label, LV_ALIGN_RIGHT_MID, -30, 30);
    lv_obj_set_style_text_color(s_description_label, C_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(s_description_label, &lv_font_montserrat_16, 0);
    lv_label_set_text(s_description_label, "Clear Sky");

    /* ── Divider ── */
    lv_obj_t *divider = lv_obj_create(card);
    lv_obj_set_size(divider, 420, 1);
    lv_obj_set_pos(divider, 20, 155);
    lv_obj_set_style_bg_color(divider, C_DIVIDER, 0);
    lv_obj_set_style_border_width(divider, 0, 0);

    /* ── Bottom: Weather Details Grid ── */
    lv_obj_t *bottom_grid = lv_obj_create(card);
    lv_obj_set_size(bottom_grid, 460, 90);
    lv_obj_set_pos(bottom_grid, 0, 160);
    lv_obj_set_style_bg_color(bottom_grid, lv_color_hex(0xFAFCFE), 0);
    lv_obj_set_style_border_width(bottom_grid, 0, 0);
    lv_obj_set_style_radius(bottom_grid, 0, 0);
    lv_obj_set_style_radius(bottom_grid, 20, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(bottom_grid, 0, 0);
    lv_obj_clear_flag(bottom_grid, LV_OBJ_FLAG_SCROLLABLE);

    // Create a flex container for the grid items
    lv_obj_t *flex_container = lv_obj_create(bottom_grid);
    lv_obj_set_size(flex_container, 440, 80);
    lv_obj_center(flex_container);
    lv_obj_set_style_bg_color(flex_container, lv_color_hex(0xFAFCFE), 0);
    lv_obj_set_style_border_width(flex_container, 0, 0);
    lv_obj_set_style_pad_all(flex_container, 5, 0);
    lv_obj_set_flex_flow(flex_container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(flex_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(flex_container, LV_OBJ_FLAG_SCROLLABLE);

    /* ── Wind ── */
    lv_obj_t *wind_container = lv_obj_create(flex_container);
    lv_obj_set_size(wind_container, 100, 60);
    lv_obj_set_style_bg_color(wind_container, lv_color_hex(0xE3F2FD), 0);
    lv_obj_set_style_radius(wind_container, 10, 0);
    lv_obj_set_style_border_width(wind_container, 0, 0);
    lv_obj_set_style_pad_all(wind_container, 0, 0);
    lv_obj_clear_flag(wind_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *wind_icon = lv_label_create(wind_container);
    lv_obj_set_pos(wind_icon, 10, 3);
    lv_obj_set_style_text_font(wind_icon, &lv_font_montserrat_16, 0);
    lv_label_set_text(wind_icon, LV_SYMBOL_LOOP);

    lv_obj_t *wind_label = lv_label_create(wind_container);
    lv_obj_set_pos(wind_label, 38, 3);
    lv_obj_set_style_text_color(wind_label, C_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(wind_label, &lv_font_montserrat_12, 0);
    lv_label_set_text(wind_label, "Wind");

    s_wind_label = lv_label_create(wind_container);
    lv_obj_set_pos(s_wind_label, 10, 28);
    lv_obj_set_style_text_color(s_wind_label, C_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(s_wind_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_wind_label, "--");

    /* ── Humidity ── */
    lv_obj_t *humidity_container = lv_obj_create(flex_container);
    lv_obj_set_size(humidity_container, 100, 60);
    lv_obj_set_style_bg_color(humidity_container, lv_color_hex(0xE8F5E9), 0);
    lv_obj_set_style_radius(humidity_container, 10, 0);
    lv_obj_set_style_border_width(humidity_container, 0, 0);
    lv_obj_set_style_pad_all(humidity_container, 0, 0);
    lv_obj_clear_flag(humidity_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *humidity_icon = lv_label_create(humidity_container);
    lv_obj_set_pos(humidity_icon, 10, 3);
    lv_obj_set_style_text_font(humidity_icon, &lv_font_montserrat_16, 0);
    lv_label_set_text(humidity_icon, LV_SYMBOL_CHARGE);

    lv_obj_t *humidity_label = lv_label_create(humidity_container);
    lv_obj_set_pos(humidity_label, 38, 3);
    lv_obj_set_style_text_color(humidity_label, C_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(humidity_label, &lv_font_montserrat_12, 0);
    lv_label_set_text(humidity_label, "Humidity");

    s_humidity_label = lv_label_create(humidity_container);
    lv_obj_set_pos(s_humidity_label, 10, 28);
    lv_obj_set_style_text_color(s_humidity_label, C_ACCENT_BLUE, 0);
    lv_obj_set_style_text_font(s_humidity_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_humidity_label, "--%");

    /* ── Visibility ── */
    lv_obj_t *visibility_container = lv_obj_create(flex_container);
    lv_obj_set_size(visibility_container, 100, 60);
    lv_obj_set_style_bg_color(visibility_container, lv_color_hex(0xFCE4EC), 0);
    lv_obj_set_style_radius(visibility_container, 10, 0);
    lv_obj_set_style_border_width(visibility_container, 0, 0);
    lv_obj_set_style_pad_all(visibility_container, 0, 0);
    lv_obj_clear_flag(visibility_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *visibility_icon = lv_label_create(visibility_container);
    lv_obj_set_pos(visibility_icon, 10, 3);
    lv_obj_set_style_text_font(visibility_icon, &lv_font_montserrat_16, 0);
    lv_label_set_text(visibility_icon, LV_SYMBOL_EYE_OPEN);

    lv_obj_t *visibility_label = lv_label_create(visibility_container);
    lv_obj_set_pos(visibility_label, 38, 3);
    lv_obj_set_style_text_color(visibility_label, C_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(visibility_label, &lv_font_montserrat_12, 0);
    lv_label_set_text(visibility_label, "Visibility");

    s_visibility_label = lv_label_create(visibility_container);
    lv_obj_set_pos(s_visibility_label, 10, 28);
    lv_obj_set_style_text_color(s_visibility_label, C_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(s_visibility_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_visibility_label, "--");

    /* ── Pressure ── */
    lv_obj_t *pressure_container = lv_obj_create(flex_container);
    lv_obj_set_size(pressure_container, 100, 60);
    lv_obj_set_style_bg_color(pressure_container, lv_color_hex(0xFFF3E0), 0);
    lv_obj_set_style_radius(pressure_container, 10, 0);
    lv_obj_set_style_border_width(pressure_container, 0, 0);
    lv_obj_set_style_pad_all(pressure_container, 0, 0);
    lv_obj_clear_flag(pressure_container, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *pressure_icon = lv_label_create(pressure_container);
    lv_obj_set_pos(pressure_icon, 10, 3);
    lv_obj_set_style_text_font(pressure_icon, &lv_font_montserrat_16, 0);
    lv_label_set_text(pressure_icon, LV_SYMBOL_LIST);

    lv_obj_t *pressure_label = lv_label_create(pressure_container);
    lv_obj_set_pos(pressure_label, 38, 3);
    lv_obj_set_style_text_color(pressure_label, C_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(pressure_label, &lv_font_montserrat_12, 0);
    lv_label_set_text(pressure_label, "Pressure");

    s_pressure_label = lv_label_create(pressure_container);
    lv_obj_set_pos(s_pressure_label, 10, 28);
    lv_obj_set_style_text_color(s_pressure_label, C_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(s_pressure_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(s_pressure_label, "--");

    /* ── Loading Indicator ── */
    s_loading_label = lv_label_create(card);
    lv_obj_align(s_loading_label, LV_ALIGN_BOTTOM_MID, 0, 5);
    lv_obj_set_style_text_color(s_loading_label, C_TEXT_LIGHT, 0);
    lv_obj_set_style_text_font(s_loading_label, &lv_font_montserrat_12, 0);
    lv_label_set_text(s_loading_label, "Fetching weather...");

    ESP_LOGI(TAG, "Modern weather UI initialized");
}