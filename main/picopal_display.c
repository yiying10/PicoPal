#include "picopal_display.h"

#include "driver/i2c_master.h"

#define PICOPAL_I2C_SDA_GPIO 8
#define PICOPAL_I2C_SCL_GPIO 9
#define PICOPAL_OLED_ADDRESS 0x3C
#define PICOPAL_I2C_FREQUENCY_HZ 100000
#define PICOPAL_DISPLAY_DATA_CONTROL_BYTE 0x40
#define PICOPAL_DISPLAY_BUFFER_SIZE 1024

static i2c_master_bus_handle_t s_bus_handle;
static i2c_master_dev_handle_t s_oled_handle;
static uint8_t s_data_transaction[PICOPAL_DISPLAY_BUFFER_SIZE + 1];

esp_err_t picopal_display_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = PICOPAL_I2C_SCL_GPIO,
        .sda_io_num = PICOPAL_I2C_SDA_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t result = i2c_new_master_bus(&bus_config, &s_bus_handle);
    if (result != ESP_OK) {
        return result;
    }

    i2c_device_config_t oled_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = PICOPAL_OLED_ADDRESS,
        .scl_speed_hz = PICOPAL_I2C_FREQUENCY_HZ,
    };

    return i2c_master_bus_add_device(
        s_bus_handle,
        &oled_config,
        &s_oled_handle
    );
}

esp_err_t picopal_display_send_command(uint8_t command)
{
    uint8_t transaction[] = { 0x00, command };

    return i2c_master_transmit(
        s_oled_handle,
        transaction,
        sizeof(transaction),
        1000
    );
}

esp_err_t picopal_display_configure(void)
{
    static const uint8_t commands[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6,
        0xAF,
    };

    for (size_t i = 0; i < sizeof(commands); ++i) {
        esp_err_t result = picopal_display_send_command(commands[i]);
        if (result != ESP_OK) {
            return result;
        }
    }

    return ESP_OK;
}

esp_err_t picopal_display_send_data(const uint8_t *data, size_t size)
{
    if (data == NULL || size > PICOPAL_DISPLAY_BUFFER_SIZE) {
        return ESP_ERR_INVALID_ARG;
    }

    s_data_transaction[0] = PICOPAL_DISPLAY_DATA_CONTROL_BYTE;
    for (size_t i = 0; i < size; ++i) {
        s_data_transaction[i + 1] = data[i];
    }

    return i2c_master_transmit(
        s_oled_handle,
        s_data_transaction,
        size + 1,
        1000
    );
}
