idf_component_register(SRCS "main.c" "ui.c" "board_init.c" 
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES 
                        bt                      
                        nvs_flash 
                        esp_wifi 
                        esp_http_client 
                        json
                        driver 
                        lvgl 
                        esp_event 
                        esp_netif 
                        esp_lcd 
                        esp_timer
)