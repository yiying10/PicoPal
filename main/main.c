#include "esp_err.h"
#include "esp_log.h"
#include "picopal_animation.h"
#include "picopal_commands.h"
#include "picopal_controller.h"
#include "picopal_display.h"
#include "picopal_events.h"
#include "picopal_framebuffer.h"
#include "picopal_input.h"
#include "picopal_i2c.h"
#include "picopal_motion.h"
#include "picopal_pages.h"
#include "picopal_pico_state.h"
#include "picopal_timer.h"
#include "picopal_touch.h"
#include "picopal_storage.h"
#include "picopal_wifi.h"

static const char *TAG = "picopal";

void app_main(void)
{
    ESP_LOGI(TAG, "PicoPal booted");

    ESP_ERROR_CHECK(picopal_storage_init());
    ESP_ERROR_CHECK(picopal_wifi_init());
    ESP_ERROR_CHECK(picopal_i2c_init());
    ESP_ERROR_CHECK(picopal_display_init());
    ESP_ERROR_CHECK(picopal_display_configure());
    ESP_ERROR_CHECK(picopal_events_init());

    picopal_timer_init();
    picopal_pico_state_init();
    picopal_pages_init();
    picopal_pages_render();
    ESP_ERROR_CHECK(picopal_framebuffer_flush());

    ESP_ERROR_CHECK(picopal_input_init());
    ESP_ERROR_CHECK(picopal_touch_init());
    ESP_ERROR_CHECK(picopal_motion_init());
    ESP_ERROR_CHECK(picopal_commands_init());
    ESP_LOGI(TAG, "PicoPal ready");

    uint64_t displayed_timer_seconds = UINT64_MAX;
    picopal_event_t event;
    while (true) {
        bool redraw = false;
        if (picopal_events_wait(&event, 33)) {
            redraw = picopal_controller_handle_event(event);
        }

        bool pico_state_changed = picopal_pico_state_update();
        bool overlay_dismissed = false;
        if (pico_state_changed && picopal_pages_overlay_active() &&
            picopal_pico_state_reaction() == PICOPAL_PICO_REACTION_NONE) {
            picopal_pages_dismiss_overlay();
            overlay_dismissed = true;
        }
        bool pico_frame_changed = picopal_animation_update();
        if ((pico_state_changed || pico_frame_changed) &&
            picopal_pages_current() == PICOPAL_PAGE_PICO) {
            redraw = true;
        }
        if (overlay_dismissed) {
            redraw = true;
        }

        if (picopal_pages_current() == PICOPAL_PAGE_TIMER) {
            uint64_t elapsed_seconds = picopal_timer_elapsed_seconds();
            if (elapsed_seconds != displayed_timer_seconds) {
                displayed_timer_seconds = elapsed_seconds;
                redraw = true;
            }
        } else {
            displayed_timer_seconds = UINT64_MAX;
        }

        if (redraw) {
            picopal_pages_render();
            ESP_ERROR_CHECK(picopal_framebuffer_flush());
        }
    }
}
