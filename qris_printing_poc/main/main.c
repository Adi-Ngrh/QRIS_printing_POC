#include "esp_log.h"
#include "wifi_config.h"
#include "driver/uart.h" /* test REQUIRES driver in CMakeList to access uart.h */

static const char *TAG = "main";

void app_main(void)
{
    uart_get_baudrate(UART_NUM_0, NULL); /* trivial call that need uart.h */

    wifi_config_init();

    if (wifi_config_connect()) {
        ESP_LOGI(TAG, "wifi connected via AT commands");
    } else {
        ESP_LOGE(TAG, "wifi AT connect failed");
    }
}
