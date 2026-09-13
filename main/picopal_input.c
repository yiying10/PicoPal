#include "picopal_input.h"

#include <stdbool.h>

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "picopal_events.h"

#define PICOPAL_JOYSTICK_X_CHANNEL ADC_CHANNEL_4
#define PICOPAL_JOYSTICK_Y_CHANNEL ADC_CHANNEL_5
#define PICOPAL_JOYSTICK_SWITCH_GPIO GPIO_NUM_7
#define PICOPAL_TIMER_BUTTON_GPIO GPIO_NUM_15

#define PICOPAL_JOYSTICK_LOW_THRESHOLD 1200
#define PICOPAL_JOYSTICK_HIGH_THRESHOLD 2900
#define PICOPAL_JOYSTICK_CENTER_LOW 1600
#define PICOPAL_JOYSTICK_CENTER_HIGH 2500
#define PICOPAL_INPUT_POLL_MS 20
#define PICOPAL_BUTTON_DEBOUNCE_SAMPLES 3

static const char *TAG = "input";
static adc_oneshot_unit_handle_t s_adc_handle;

typedef struct {
    gpio_num_t gpio;
    picopal_event_type_t event;
    bool candidate_pressed;
    bool stable_pressed;
    uint8_t matching_samples;
} button_input_t;

static bool value_is_centered(int value)
{
    return value >= PICOPAL_JOYSTICK_CENTER_LOW &&
           value <= PICOPAL_JOYSTICK_CENTER_HIGH;
}

static void publish_direction(picopal_event_type_t type)
{
    if (!picopal_events_publish((picopal_event_t){ .type = type })) {
        ESP_LOGW(TAG, "Event queue full; input discarded");
    }
}

static void update_button(button_input_t *button)
{
    bool pressed = gpio_get_level(button->gpio) == 0;
    if (pressed == button->candidate_pressed) {
        if (button->matching_samples < PICOPAL_BUTTON_DEBOUNCE_SAMPLES) {
            ++button->matching_samples;
        }
    } else {
        button->candidate_pressed = pressed;
        button->matching_samples = 1;
    }

    if (button->matching_samples >= PICOPAL_BUTTON_DEBOUNCE_SAMPLES &&
        button->stable_pressed != button->candidate_pressed) {
        button->stable_pressed = button->candidate_pressed;
        if (button->stable_pressed) {
            publish_direction(button->event);
        }
    }
}

static void input_task(void *context)
{
    (void)context;
    bool armed = true;
    button_input_t joystick_switch = {
        .gpio = PICOPAL_JOYSTICK_SWITCH_GPIO,
        .event = PICOPAL_EVENT_TIMER_RESET,
    };
    button_input_t timer_button = {
        .gpio = PICOPAL_TIMER_BUTTON_GPIO,
        .event = PICOPAL_EVENT_TIMER_TOGGLE,
    };

    while (true) {
        update_button(&joystick_switch);
        update_button(&timer_button);

        int x_raw = 0;
        int y_raw = 0;
        esp_err_t x_result = adc_oneshot_read(
            s_adc_handle,
            PICOPAL_JOYSTICK_X_CHANNEL,
            &x_raw
        );
        esp_err_t y_result = adc_oneshot_read(
            s_adc_handle,
            PICOPAL_JOYSTICK_Y_CHANNEL,
            &y_raw
        );

        if (x_result != ESP_OK || y_result != ESP_OK) {
            ESP_LOGW(TAG, "Joystick ADC read failed");
            vTaskDelay(pdMS_TO_TICKS(PICOPAL_INPUT_POLL_MS));
            continue;
        }

        if (!armed && value_is_centered(x_raw) && value_is_centered(y_raw)) {
            armed = true;
        } else if (armed) {
            if (x_raw < PICOPAL_JOYSTICK_LOW_THRESHOLD) {
                publish_direction(PICOPAL_EVENT_DRAW_PREVIOUS);
                armed = false;
            } else if (x_raw > PICOPAL_JOYSTICK_HIGH_THRESHOLD) {
                publish_direction(PICOPAL_EVENT_DRAW_NEXT);
                armed = false;
            } else if (y_raw < PICOPAL_JOYSTICK_LOW_THRESHOLD) {
                publish_direction(PICOPAL_EVENT_PAGE_PREVIOUS);
                armed = false;
            } else if (y_raw > PICOPAL_JOYSTICK_HIGH_THRESHOLD) {
                publish_direction(PICOPAL_EVENT_PAGE_NEXT);
                armed = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(PICOPAL_INPUT_POLL_MS));
    }
}

esp_err_t picopal_input_init(void)
{
    adc_oneshot_unit_init_cfg_t unit_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_RETURN_ON_ERROR(
        adc_oneshot_new_unit(&unit_config, &s_adc_handle),
        TAG,
        "ADC unit initialization failed"
    );

    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_RETURN_ON_ERROR(
        adc_oneshot_config_channel(
            s_adc_handle,
            PICOPAL_JOYSTICK_X_CHANNEL,
            &channel_config
        ),
        TAG,
        "Joystick X configuration failed"
    );
    ESP_RETURN_ON_ERROR(
        adc_oneshot_config_channel(
            s_adc_handle,
            PICOPAL_JOYSTICK_Y_CHANNEL,
            &channel_config
        ),
        TAG,
        "Joystick Y configuration failed"
    );

    gpio_config_t button_config = {
        .pin_bit_mask = (1ULL << PICOPAL_JOYSTICK_SWITCH_GPIO) |
            (1ULL << PICOPAL_TIMER_BUTTON_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(
        gpio_config(&button_config),
        TAG,
        "Button configuration failed"
    );

    BaseType_t created = xTaskCreate(
        input_task,
        "picopal_input",
        3072,
        NULL,
        5,
        NULL
    );
    return created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
