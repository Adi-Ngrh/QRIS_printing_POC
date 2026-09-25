#include <stdio.h>
#include "cJSON.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt_setup.h"
#include "printer_forward.h"

/* Code to connect to the MQTT broker and subscribe to topic-notif-{device_id} */

#define MQTT_BROKER_URI  "mqtt://10.170.161.230:1883"

/* unique id for each device */
#define DEVICE_IMEI      "3"
static const char *TAG = "mqtt_setup";

/* buffer for topic name */
static char s_notif_topic[32];

/*
* @brief parser function
*
* turn received payload stream into structured JSON
*
* @param json : received payload stream
* @param len : size of payload
* @param out : the output (parsed)
*/
static void parse_payment_payload(const char *json, int len, printer_receipt_t *out, char *qr_content, size_t qr_content_size, int *print_mode)
{
    static char amount[32], invoice_number[64], paid_at[32], serial[32], status[16];

    cJSON *root = cJSON_ParseWithLength(json, len);

    snprintf(amount, sizeof(amount), "%s", cJSON_GetObjectItem(root, "amount")->valuestring);
    snprintf(invoice_number, sizeof(invoice_number), "%s", cJSON_GetObjectItem(root, "invoice_number")->valuestring);
    snprintf(paid_at, sizeof(paid_at), "%s", cJSON_GetObjectItem(root, "paid_at")->valuestring);
    snprintf(serial, sizeof(serial), "%s", cJSON_GetObjectItem(root, "serial")->valuestring);
    snprintf(status, sizeof(status), "%s", cJSON_GetObjectItem(root, "status")->valuestring);
    snprintf(qr_content, qr_content_size, "%s", cJSON_GetObjectItem(root, "qr_content")->valuestring);
    *print_mode = cJSON_GetObjectItem(root, "print_mode")->valueint;

    cJSON_Delete(root);

    out->amount = amount;
    out->invoice_number = invoice_number;
    out->paid_at = paid_at;
    out->serial = serial;
    out->status = status;
}

/*
* @brief MQTT event handler
*
* called continuously by the event loop in MQTT client to receive MQTT event
*
* @param arg : information about user registered to the received event
* @param event_base : name of the received event
* @param event_id : id number of the received event
* @param event_data : information about the received event
*/
static void mqtt_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch (event->event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "connected to broker, subscribing to %s", s_notif_topic);
        esp_mqtt_client_subscribe(event->client, s_notif_topic, 1);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "message on %.*s: %.*s", event->topic_len, event->topic, event->data_len, event->data);
        printer_receipt_t receipt;
        char qris[300];
        int print_mode;
        parse_payment_payload(event->data, event->data_len, &receipt, qris, sizeof(qris), &print_mode);

        if (print_mode == 0) {
            printer_print_receipt(&receipt);
        } else if (print_mode == 1) {
            printer_print_qris(qris);
        } else {
            printer_print_raw((const uint8_t *)event->data, event->data_len);
        }
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "disconnected from broker");
        break;

    default:
        break;
    }
}

/*
* @brief setup MQTT connection and register MQTT event handler
*/
void mqtt_config_start(void)
{
    snprintf(s_notif_topic, sizeof(s_notif_topic), "topic-notif-%s", DEVICE_IMEI);

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    /* open MQTT connection */
    esp_mqtt_client_start(client);
}
