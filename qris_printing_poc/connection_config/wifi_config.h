#pragma once

#include <stdbool.h>

/* STEP 1: bring up NVS + the network stack + the WiFi driver, station mode. */
void wifi_config_init(void);

/* STEP 2-3: connect to the configured AP and block until connected/failed. */
bool wifi_config_connect(void);
