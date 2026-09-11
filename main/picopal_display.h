#pragma once

#include <stdint.h>
#include <stddef.h>

#include "esp_err.h"

esp_err_t picopal_display_init(void);
esp_err_t picopal_display_send_command(uint8_t command);
esp_err_t picopal_display_configure(void);
esp_err_t picopal_display_send_data(const uint8_t *data, size_t size);
