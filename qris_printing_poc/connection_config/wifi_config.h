#pragma once

#include <stdbool.h>

/* STEP 1: open the UART port that the Wi-Fi module is wired to. */
void wifi_config_init(void);

/* STEP 3: run the connection sequence (AT test, station mode, join AP). */
bool wifi_config_connect(void);
