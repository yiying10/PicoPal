#include "picopal_motion.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "picopal_events.h"
#include "picopal_i2c.h"

#define MPU6050_ADDRESS 0x68
#define MPU6050_I2C_HZ 400000
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_WHO_AM_I 0x75

#define MOTION_POLL_MS 20
#define MOTION_DELTA_THRESHOLD 6500
#define MOTION_WINDOW_MS 420
#define MOTION_SHAKE_HITS 5
#define MOTION_COOLDOWN_MS 700

static const char *TAG = "motion";
static i2c_master_dev_handle_t s_mpu6050;

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} acceleration_t;

static esp_err_t write_register(uint8_t reg, uint8_t value)
{
    uint8_t payload[] = {reg, value};
    return i2c_master_transmit(
        s_mpu6050,
        payload,
        sizeof(payload),
        100
    );
}

static esp_err_t read_registers(uint8_t reg, uint8_t *data, size_t length)
{
    return i2c_master_transmit_receive(
        s_mpu6050,
        &reg,
        sizeof(reg),
        data,
        length,
        100
    );
}

static esp_err_t read_acceleration(acceleration_t *acceleration)
{
    uint8_t bytes[6];
    ESP_RETURN_ON_ERROR(
        read_registers(MPU6050_REG_ACCEL_XOUT_H, bytes, sizeof(bytes)),
        TAG,
        "Accelerometer read failed"
    );

    acceleration->x = (int16_t)(((uint16_t)bytes[0] << 8) | bytes[1]);
    acceleration->y = (int16_t)(((uint16_t)bytes[2] << 8) | bytes[3]);
    acceleration->z = (int16_t)(((uint16_t)bytes[4] << 8) | bytes[5]);
    return ESP_OK;
}

static int32_t absolute_i32(int32_t value)
{
    return value < 0 ? -value : value;
}

static int32_t acceleration_delta(
    const acceleration_t *current,
    const acceleration_t *previous
)
{
    return absolute_i32((int32_t)current->x - previous->x) +
        absolute_i32((int32_t)current->y - previous->y) +
        absolute_i32((int32_t)current->z - previous->z);
}

static void publish_motion(picopal_event_type_t type, uint8_t hits)
{
    if (!picopal_events_publish((picopal_event_t){.type = type})) {
        ESP_LOGW(TAG, "Event queue full; motion discarded");
        return;
    }
    ESP_LOGI(
        TAG,
        "%s detected (%u hits)",
        type == PICOPAL_EVENT_MOTION_SHAKE ? "Shake" : "Tap",
        hits
    );
}

static void motion_task(void *context)
{
    (void)context;
    acceleration_t previous;
    if (read_acceleration(&previous) != ESP_OK) {
        vTaskDelete(NULL);
        return;
    }

    bool window_active = false;
    uint8_t motion_hits = 0;
    int64_t window_started_us = 0;
    int64_t cooldown_until_us = 0;

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(MOTION_POLL_MS));

        acceleration_t current;
        if (read_acceleration(&current) != ESP_OK) {
            continue;
        }

        int64_t now_us = esp_timer_get_time();
        int32_t delta = acceleration_delta(&current, &previous);
        previous = current;

        if (now_us < cooldown_until_us) {
            continue;
        }

        if (delta >= MOTION_DELTA_THRESHOLD) {
            if (!window_active) {
                window_active = true;
                window_started_us = now_us;
                motion_hits = 0;
            }
            if (motion_hits < UINT8_MAX) {
                ++motion_hits;
            }
        }

        if (window_active &&
            now_us - window_started_us >= MOTION_WINDOW_MS * 1000LL) {
            picopal_event_type_t type =
                motion_hits >= MOTION_SHAKE_HITS
                    ? PICOPAL_EVENT_MOTION_SHAKE
                    : PICOPAL_EVENT_MOTION_TAP;
            publish_motion(type, motion_hits);
            window_active = false;
            cooldown_until_us = now_us + MOTION_COOLDOWN_MS * 1000LL;
        }
    }
}

esp_err_t picopal_motion_init(void)
{
    ESP_RETURN_ON_ERROR(
        picopal_i2c_add_device(
            MPU6050_ADDRESS,
            MPU6050_I2C_HZ,
            &s_mpu6050
        ),
        TAG,
        "MPU6050 registration failed"
    );

    uint8_t identity = 0;
    ESP_RETURN_ON_ERROR(
        read_registers(MPU6050_REG_WHO_AM_I, &identity, sizeof(identity)),
        TAG,
        "MPU6050 not found"
    );
    if ((identity & 0x7E) != 0x68) {
        ESP_LOGE(TAG, "Unexpected WHO_AM_I: 0x%02X", identity);
        return ESP_ERR_NOT_FOUND;
    }

    ESP_RETURN_ON_ERROR(
        write_register(MPU6050_REG_PWR_MGMT_1, 0x00),
        TAG,
        "MPU6050 wake failed"
    );
    vTaskDelay(pdMS_TO_TICKS(100));

    BaseType_t created = xTaskCreate(
        motion_task,
        "picopal_motion",
        3072,
        NULL,
        4,
        NULL
    );
    if (created != pdPASS) {
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "MPU6050 ready at 0x%02X", MPU6050_ADDRESS);
    return ESP_OK;
}
