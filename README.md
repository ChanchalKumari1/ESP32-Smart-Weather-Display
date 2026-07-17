# ESP32-Smart-Weather-Display - ELECROW ESP32 display-4.3 Inch based
ELECROW ESP32 display-4.3 Inch HMI Display 480x272 RGB TFT LCD Touch Screen Compatible with Arduino/LVGL/PlatformIO/ Micropython Without Acrylic Case

# ESP32 Smart Weather Display

An ESP32-based smart weather dashboard developed using the **ESP-IDF framework**. The project retrieves live weather information from the OpenWeather API and displays it on an Elecrow ESP32 touchscreen display.

Wi-Fi credentials, latitude, and longitude are configured wirelessly using the **BLE WiFi Setup – IoT Devices** mobile application. The application communicates with the ESP32 through Bluetooth Low Energy, removing the need to hardcode Wi-Fi or location information in the firmware.

## Features

* Developed using ESP-IDF
* Elecrow ESP32 display support
* Live weather data using the OpenWeather API
* Wi-Fi provisioning through Bluetooth Low Energy
* Latitude and longitude configuration through BLE
* No hardcoded Wi-Fi credentials
* Automatic weather-data refresh
* JSON response parsing
* Non-volatile configuration storage
* Weather information displayed through a graphical interface
* Wi-Fi and API connection-status handling

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

## System Workflow

1. The ESP32 initializes the display, BLE service, NVS, and other required components.
2. The ESP32 starts advertising as a BLE device.
3. The user opens the BLE WiFi Setup application.
4. The application scans for and connects to the ESP32.
5. The user selects the required Wi-Fi network.
6. The application sends the Wi-Fi SSID and password through BLE.
7. The application sends the latitude and longitude coordinates.
8. The ESP32 stores the configuration in NVS.
9. The ESP32 connects to the configured Wi-Fi network.
10. The ESP32 sends an HTTPS request to the OpenWeather API.
11. The received JSON response is parsed.
12. The weather information is shown on the Elecrow display.
13. Weather data is refreshed automatically at a configured interval.

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

## License

This project is open source and intended for ESP-IDF learning, IoT development, BLE provisioning, and weather API integration.


https://drive.google.com/drive/folders/1ILlToPnGBnjLAOv5CfqYjuUL5mR0pnwP?usp=sharing
