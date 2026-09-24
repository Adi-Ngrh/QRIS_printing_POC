#include "esp_log.h"
#include "wifi_config.h"
#include "mqtt_setup.h"
#include "printer_forward.h"

static const char *TAG = "main";

void app_main(void)
{
    wifi_config_init();

    if (wifi_config_connect()) {
        ESP_LOGI(TAG, "wifi connected");
        printer_forward_init();
        mqtt_config_start();
    } else {
        ESP_LOGE(TAG, "wifi connection failed");
    }
}
