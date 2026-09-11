#pragma once

#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

esp_err_t picopal_i2c_init(void);
esp_err_t picopal_i2c_add_device(
    uint16_t address,
    uint32_t frequency_hz,
    i2c_master_dev_handle_t *device_handle
);
