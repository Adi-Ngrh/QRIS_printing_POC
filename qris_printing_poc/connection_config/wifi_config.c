#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "wifi_config.h"

/*
 * GOAL: connect to a Wi-Fi access point using AT commands sent over UART,
 * the same way an MCU with no built-in radio (e.g. NXP + FC41D) would.
 * Nothing else is handled here (no data send/receive, no reconnection logic).
 */

#define AT_UART_PORT     UART_NUM_1
#define AT_UART_TX_PIN   17
#define AT_UART_RX_PIN   16
#define AT_UART_BAUD     115200
#define AT_BUF_SIZE      512

#define WIFI_SSID   "your-ssid"
#define WIFI_PASS   "your-password"

static const char *TAG = "wifi_at";

/* STEP 1: open the UART port that the Wi-Fi module is wired to. */
void wifi_config_init(void)
{
    uart_config_t cfg = {
        .baud_rate = AT_UART_BAUD,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(AT_UART_PORT, &cfg);
    uart_set_pin(AT_UART_PORT, AT_UART_TX_PIN, AT_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(AT_UART_PORT, AT_BUF_SIZE * 2, 0, 0, NULL, 0);
}

/*
 * STEP 2: send one AT command and wait for the module's reply.
 * Every AT exchange follows this same request/response shape, so this
 * helper is reused for each command in the connection flow below.
 */
static bool at_send_cmd(const char *cmd, uint32_t timeout_ms)
{
    uint8_t rx[AT_BUF_SIZE] = {0};

    uart_write_bytes(AT_UART_PORT, cmd, strlen(cmd));
    uart_write_bytes(AT_UART_PORT, "\r\n", 2);

    int len = uart_read_bytes(AT_UART_PORT, rx, sizeof(rx) - 1, pdMS_TO_TICKS(timeout_ms));
    if (len <= 0) {
        ESP_LOGE(TAG, "no response to: %s", cmd);
        return false;
    }

    rx[len] = '\0';
    ESP_LOGI(TAG, "resp: %s", rx);
    return strstr((char *)rx, "OK") != NULL;
}

/* STEP 3: run the actual connection sequence, one AT command per stage. */
bool wifi_config_connect(void)
{
    char cmd[128];

    /* check the module powered up and respond status. */
    if (!at_send_cmd("AT", 1000)) return false;

    /* put the module into station mode (client joining an AP). */
    if (!at_send_cmd("AT+CWMODE=1", 1000)) return false;

    /* join the target AP with SSID/password. This is the slow step. */
    snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASS);
    if (!at_send_cmd(cmd, 10000)) return false;

    return true;
}
