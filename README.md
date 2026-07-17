# ESP32-Smart-Weather-Display - ELECROW ESP32 display-4.3 Inch based
ELECROW ESP32 display-4.3 Inch HMI Display 480x272 RGB TFT LCD Touch Screen Compatible with Arduino/LVGL/PlatformIO/ Micropython Without Acrylic Case

# ESP32 Smart Weather Display

An ESP32-based smart weather dashboard developed using the **ESP-IDF framework**. The project retrieves live weather information from the OpenWeather API and displays it on an Elecrow ESP32 touchscreen display.

Wi-Fi credentials, latitude, and longitude are configured wirelessly using the **BLE WiFi Setup – IoT Devices** mobile application. The application communicates with the ESP32 through Bluetooth Low Energy, removing the need to hardcode Wi-Fi or location information in the firmware.

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

## Mobile Configuration Application

This project uses the following iOS application:

**BLE WiFi Setup – IoT Devices**

The application is used to:

* Scan for the ESP32 BLE device
* Connect to the ESP32
* Select a Wi-Fi network
* Send the Wi-Fi SSID
* Send the Wi-Fi password
* Send latitude and longitude
* Send additional configuration data
* Reconfigure the device without reflashing the firmware

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


## Displayed Information

* Current temperature
* Feels-like temperature
* Minimum and maximum temperature
* Weather condition
* Weather description
* Humidity
* Atmospheric pressure
* Wind speed
* City or location name
* Weather icon
* Wi-Fi connection status

## Software Components

* ESP-IDF
* FreeRTOS
* ESP-IDF Wi-Fi
* ESP-IDF Bluetooth Low Energy
* ESP HTTP Client
* ESP TLS
* cJSON
* NVS Flash
* LVGL
* OpenWeather API

## Hardware

* Elecrow ESP32 display
* Integrated ESP32 microcontroller
* Wi-Fi connectivity
* Bluetooth Low Energy
* USB power supply

## Applications

* Smart desk weather display
* Home weather dashboard
* Office information display
* ESP-IDF BLE provisioning example
* OpenWeather API integration example
* BLE and Wi-Fi IoT demonstration
* Embedded graphical-interface development

## Future Improvements

* Multi-day weather forecast
* Sunrise and sunset information
* Air Quality Index
* UV index
* NTP date and time
* Weather alerts
* Touchscreen configuration
* Indoor temperature and humidity sensors

### License

This project is open source and intended for ESP-IDF learning, IoT development, BLE provisioning, and weather API integration.


https://drive.google.com/drive/folders/1ILlToPnGBnjLAOv5CfqYjuUL5mR0pnwP?usp=sharing
