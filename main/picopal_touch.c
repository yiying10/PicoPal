#include "picopal_touch.h"

#include <stdbool.h>
#include <inttypes.h>
#include <stdint.h>

#include "driver/touch_sensor.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "picopal_events.h"

#define PICOPAL_TOUCH_PAD TOUCH_PAD_NUM4
#define PICOPAL_TOUCH_CALIBRATION_SAMPLES 32
#define PICOPAL_TOUCH_POLL_MS 20
#define PICOPAL_TOUCH_DEBOUNCE_SAMPLES 3
#define PICOPAL_TOUCH_LONG_PRESS_MS 1200
#define PICOPAL_TOUCH_MINIMUM_PRESS_MS 60

static const char *TAG = "touch";

static bool publish_touch_event(picopal_event_type_t type)
{
    bool published = picopal_events_publish((picopal_event_t){
        .type = type,
    });
    if (!published) {
        ESP_LOGW(TAG, "Event queue full; touch discarded");
    }
    return published;
}

static bool read_touch_value(uint32_t *value)
{
    esp_err_t result = touch_pad_read_raw_data(PICOPAL_TOUCH_PAD, value);
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "Touch read failed: %s", esp_err_to_name(result));
        return false;
    }
    return true;
}

static bool calibrate_baseline(uint32_t *baseline)
{
    uint64_t sum = 0;
    uint32_t valid_samples = 0;

    vTaskDelay(pdMS_TO_TICKS(100));
    for (uint32_t i = 0; i < PICOPAL_TOUCH_CALIBRATION_SAMPLES; ++i) {
        uint32_t value = 0;
        if (read_touch_value(&value)) {
            sum += value;
            ++valid_samples;
        }
        vTaskDelay(pdMS_TO_TICKS(PICOPAL_TOUCH_POLL_MS));
    }

    if (valid_samples == 0) {
        return false;
    }
    *baseline = (uint32_t)(sum / valid_samples);
    ESP_LOGI(TAG, "Touch baseline: %" PRIu32, *baseline);
    return true;
}

static void touch_task(void *context)
{
    (void)context;
    uint32_t baseline = 0;
    if (!calibrate_baseline(&baseline)) {
        ESP_LOGE(TAG, "Touch calibration failed");
        vTaskDelete(NULL);
        return;
    }

    bool stable_touched = false;
    bool candidate_touched = false;
    uint8_t matching_samples = 0;
    int64_t touch_started_us = 0;

    while (true) {
        uint32_t raw = 0;
        if (!read_touch_value(&raw)) {
            vTaskDelay(pdMS_TO_TICKS(PICOPAL_TOUCH_POLL_MS));
            continue;
        }

        bool touched = raw < (baseline * 75) / 100U;
        if (touched == candidate_touched) {
            if (matching_samples < PICOPAL_TOUCH_DEBOUNCE_SAMPLES) {
                ++matching_samples;
            }
        } else {
            candidate_touched = touched;
            matching_samples = 1;
        }

        if (matching_samples >= PICOPAL_TOUCH_DEBOUNCE_SAMPLES &&
            stable_touched != candidate_touched) {
            stable_touched = candidate_touched;
            if (stable_touched) {
                touch_started_us = esp_timer_get_time();
            } else {
                int64_t duration_ms =
                    (esp_timer_get_time() - touch_started_us) / 1000;
                if (duration_ms >= PICOPAL_TOUCH_LONG_PRESS_MS) {
                    publish_touch_event(PICOPAL_EVENT_TOUCH_LONG);
                } else if (duration_ms >= PICOPAL_TOUCH_MINIMUM_PRESS_MS) {
                    publish_touch_event(PICOPAL_EVENT_TOUCH_SHORT);
                }
            }
        }

        if (!stable_touched && !candidate_touched) {
            baseline = (baseline * 31U + raw) / 32U;
        }
        vTaskDelay(pdMS_TO_TICKS(PICOPAL_TOUCH_POLL_MS));
    }
}

esp_err_t picopal_touch_init(void)
{
    ESP_RETURN_ON_ERROR(touch_pad_init(), TAG, "Touch init failed");
    ESP_RETURN_ON_ERROR(
        touch_pad_config(PICOPAL_TOUCH_PAD),
        TAG,
        "Touch channel config failed"
    );
    ESP_RETURN_ON_ERROR(
        touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER),
        TAG,
        "Touch FSM config failed"
    );
    ESP_RETURN_ON_ERROR(
        touch_pad_fsm_start(),
        TAG,
        "Touch FSM start failed"
    );

    BaseType_t created = xTaskCreate(
        touch_task,
        "picopal_touch",
        3072,
        NULL,
        4,
        NULL
    );
    return created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
