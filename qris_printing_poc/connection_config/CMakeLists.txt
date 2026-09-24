idf_component_register(SRCS "mqtt_setup.c" "wifi_config.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_wifi esp_event nvs_flash mqtt printer_code cjson log) #[[other components beside "main" need explicit requirements declaration]]