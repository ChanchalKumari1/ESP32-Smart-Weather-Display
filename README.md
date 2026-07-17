# ESP32 BLE Weather Station

An ESP32-S3 firmware project (ESP-IDF) that drives a 480x272 RGB LCD ("CrowPanel 4.3\"") with a
resistive touch panel, fetches live weather from OpenWeatherMap over Wi-Fi, and lets a phone
configure Wi-Fi credentials and location over Bluetooth LE — no serial console or reflashing
needed to point it at a new network or city.

The project retrieves live weather information from the OpenWeather API and displays it on an Elecrow ESP32 touchscreen display.

Wi-Fi credentials, latitude, and longitude are configured wirelessly using the **BLE WiFi Setup – IoT Devices mobile application**. The application communicates with the ESP32 through Bluetooth Low Energy, removing the need to hardcode Wi-Fi or location information in the firmware.

## Features

- **Live weather display** — temperature, description, humidity, wind (speed + compass
  direction), visibility, and pressure, rendered on an LVGL card UI with a small vector-drawn
  weather icon (sun / clouds / rain / thunder / snow / mist) that updates with each fetch.
- **Wi-Fi provisioning over BLE** — write `{"ssid":"...","password":"..."}` to a GATT
  characteristic and the device reconnects and persists the credentials to NVS.
- **Location provisioning over BLE** — write `{"lat":31.3256,"lon":75.5792}` to switch the
  weather query location on the fly; also persisted to NVS.
- **Resilient networking** — automatic Wi-Fi reconnect on disconnect, HTTP fetch retries
  (up to 3 attempts, 5s apart) on failure.
- **Resistive touch** — bit-banged XPT2046 SPI driver feeding LVGL's input device layer
  (currently wired up but the UI has no interactive controls yet).

## Hardware

- ESP32-S3 with PSRAM (frame buffer lives in PSRAM)
- 480x272 RGB parallel LCD (16-bit RGB565 bus)
- XPT2046 resistive touch controller, sharing the SD card SPI bus
- PWM-controlled backlight

Pin assignments, timing parameters, and resolution are all defined in `board_init.h`.

## Project Layout

| File | Purpose |
|---|---|
| `main.c` | App entry point (`app_main`), NVS-backed config load/save, task bring-up |
| `defines.h` | Shared includes, constants, `weather_data_t` / `app_config_t` structs, function prototypes |
| `board_init.c` / `board_init.h` | LCD panel + backlight + touch + LVGL driver bring-up, pin map |
| `wifi.h` | Wi-Fi station init, event handling, reconnect, DNS setup |
| `api.h` | OpenWeatherMap HTTP client (async fetch task, JSON parsing into `weather_data_t`) |
| `ble.h` | NimBLE GATT server: Wi-Fi and location provisioning characteristics |
| `ui.c` / `ui.h` | LVGL weather card UI, including the vector weather icon renderer |

## How It Works

1. `app_main()` loads any saved config from NVS (falls back to defaults in `defines.h`),
   brings up Wi-Fi, the display/touch stack, the weather API task, and the UI.
2. Once Wi-Fi has an IP, `api_set_coords()` kicks off the first weather fetch.
3. `api_fetch_task` polls a "pending" flag, performs the HTTP GET, and parses the JSON
   response into the global `s_weather_data` struct, retrying on failure.
4. On a successful parse, `ui_weather_update()` is called to push the new data into the
   LVGL widgets, including redrawing the vector weather icon for the current condition.
5. BLE stays up throughout, advertising as `WeatherStation`; writing to the Wi-Fi or
   location characteristic updates config in NVS and triggers a reconnect / re-fetch
   immediately, without a reboot.

## BLE provisioning payloads

Write these as UTF-8 JSON to the relevant characteristic (curly quotes from mobile
keyboards are auto-normalized to straight quotes):

```jsonc
// WiFi characteristic (6e400002-b5a3-f393-e0a9-e50e24dcca9e)
{"ssid":"MyNetwork","password":"MyPassword"}

// Location characteristic (beb5483e-36e1-4688-b7f5-ea07361b26a8)
{"lat":31.3256,"lon":75.5792}
```

Service UUID: `4fafc201-1fb5-459e-8fcc-c5c9c331914b`

## Weather icon mapping

`ui.c` draws small icons directly with LVGL primitives (circles + lines) — no image or
custom font assets required. `api.h` buckets the raw OpenWeatherMap condition code into
groups before storing it in `weather_data_t.weather_icon_id`, which `ui.c` then maps to a
graphic:

| Bucket | Condition | Icon |
|---|---|---|
| 8 | Clear sky | Sun (glow + solid circle) |
| 2 / 3 | Few / scattered clouds | Cloud |
| 4 | Broken clouds | Two overlapping clouds |
| 5 | Rain | Cloud + 3 raindrops |
| 9 | Heavy rain | Cloud + 5 raindrops |
| 10 / 11 | Thunderstorm | Cloud + lightning bolt |
| 13 | Snow | Cloud + snowflake dots |
| 50 | Mist | Horizontal haze lines |

## Configuration

Edit `defines.h` before building:

- `WIFI_DEFAULT_SSID` / `WIFI_DEFAULT_PASSWORD` — fallback Wi-Fi credentials used until BLE
  provisioning overrides them
- `DEFAULT_LAT` / `DEFAULT_LON` — fallback location
- `WEATHER_API_KEY` — your OpenWeatherMap API key
- `BLE_DEVICE_NAME` — advertised BLE device name

> **Note:** `WEATHER_API_KEY` is currently committed in plaintext in `defines.h`. If this
> repo is or will be public, rotate the key and load it from NVS, `menuconfig`, or a
> gitignored header instead.

## Building

Standard ESP-IDF project layout is assumed (this README doesn't include `CMakeLists.txt` /
`sdkconfig`, which should already exist in your IDF project structure).

```bash
idf.py set-target esp32s3
idf.py menuconfig   # configure PSRAM, partition table, etc. if not already set
idf.py build
idf.py -p <PORT> flash monitor
```

## Known Limitations / Ideas for Follow-up

- Touch input is wired to LVGL but no on-screen controls consume it yet (e.g. a settings
  screen for Wi-Fi/location as an alternative to BLE).
- `WEATHER_API_KEY` should be moved out of source control (see note above).
- No BLE pairing/bonding is configured — the GATT characteristics are writable by any
  connected device. Consider adding authentication if the device will be used somewhere
  with untrusted BLE proximity.
- The weather fetch interval is manual (BLE-triggered or on Wi-Fi reconnect) — there's no
  periodic re-fetch timer yet if you want the display to refresh automatically over time.


 ## Videos and Images : 
https://drive.google.com/drive/folders/1ILlToPnGBnjLAOv5CfqYjuUL5mR0pnwP?usp=sharing
