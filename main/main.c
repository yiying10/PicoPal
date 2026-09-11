#include "esp_err.h"
#include "esp_log.h"
#include "picopal_animation.h"
#include "picopal_commands.h"
#include "picopal_display.h"
#include "picopal_events.h"
#include "picopal_framebuffer.h"
#include "picopal_input.h"
#include "picopal_i2c.h"
#include "picopal_pages.h"
#include "picopal_pico_state.h"
#include "picopal_timer.h"
#include "picopal_touch.h"

static const char *TAG = "picopal";

void app_main(void)
{
    ESP_LOGI(TAG, "PicoPal booted");

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
    ESP_ERROR_CHECK(picopal_commands_init());
    ESP_LOGI(TAG, "PicoPal ready");

    uint64_t displayed_timer_seconds = UINT64_MAX;
    picopal_event_t event;
    while (true) {
        bool redraw = false;
        if (picopal_events_wait(&event, 33)) {
            if (event.type == PICOPAL_EVENT_TIMER_TOGGLE &&
                picopal_pages_current() == PICOPAL_PAGE_TIMER &&
                !picopal_pages_overlay_active()) {
                picopal_timer_toggle();
                redraw = true;
            } else if (event.type == PICOPAL_EVENT_PICO_SET_BASE) {
                picopal_pico_state_set_base((picopal_pico_base_t)event.value);
                redraw = picopal_pages_current() == PICOPAL_PAGE_PICO;
            } else if (event.type == PICOPAL_EVENT_PICO_REACTION) {
                picopal_pico_reaction_t reaction =
                    (picopal_pico_reaction_t)event.value;
                bool accepted = picopal_pico_state_start_reaction(
                    reaction
                );
                if (accepted && reaction == PICOPAL_PICO_REACTION_CODEX_DONE &&
                    picopal_pages_current() != PICOPAL_PAGE_PICO) {
                    picopal_pages_show_pico_overlay();
                }
                redraw = accepted &&
                    (picopal_pages_current() == PICOPAL_PAGE_PICO ||
                     picopal_pages_overlay_active());
            } else if (event.type == PICOPAL_EVENT_PICO_STATUS) {
                ESP_LOGI(
                    TAG,
                    "Pico base=%d reaction=%d",
                    picopal_pico_state_base(),
                    picopal_pico_state_reaction()
                );
            } else if (event.type == PICOPAL_EVENT_TOUCH_SHORT ||
                       event.type == PICOPAL_EVENT_TOUCH_LONG) {
                picopal_pico_base_t base = picopal_pico_state_base();
                if (base == PICOPAL_PICO_BASE_ERROR) {
                    ESP_LOGW(TAG, "Touch ignored while Pico is in error");
                } else if (base == PICOPAL_PICO_BASE_SLEEP) {
                    picopal_pico_state_set_base(PICOPAL_PICO_BASE_IDLE);
                    picopal_pico_state_start_reaction(
                        PICOPAL_PICO_REACTION_WAKE
                    );
                } else if (event.type == PICOPAL_EVENT_TOUCH_LONG) {
                    picopal_pico_state_set_base(PICOPAL_PICO_BASE_SLEEP);
                } else {
                    picopal_pico_state_start_reaction(
                        PICOPAL_PICO_REACTION_HAPPY
                    );
                }
                redraw = base != PICOPAL_PICO_BASE_ERROR &&
                    picopal_pages_current() == PICOPAL_PAGE_PICO;
            } else {
                redraw = picopal_pages_handle_event(event);
            }
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
