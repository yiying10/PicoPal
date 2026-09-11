#include "picopal_i2c.h"

#define PICOPAL_I2C_SDA_GPIO 8
#define PICOPAL_I2C_SCL_GPIO 9

static i2c_master_bus_handle_t s_bus_handle;

esp_err_t picopal_i2c_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = PICOPAL_I2C_SCL_GPIO,
        .sda_io_num = PICOPAL_I2C_SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(&bus_config, &s_bus_handle);
}

esp_err_t picopal_i2c_add_device(
    uint16_t address,
    uint32_t frequency_hz,
    i2c_master_dev_handle_t *device_handle
)
{
    if (s_bus_handle == NULL || device_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t device_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = address,
        .scl_speed_hz = frequency_hz,
    };

    return i2c_master_bus_add_device(s_bus_handle, &device_config, device_handle);
}
