#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef enum {
    PICOPAL_WIFI_MODE_SETUP,
    PICOPAL_WIFI_MODE_STATION,
} picopal_wifi_mode_t;

esp_err_t picopal_wifi_init(void);
picopal_wifi_mode_t picopal_wifi_mode(void);
bool picopal_wifi_connected(void);
