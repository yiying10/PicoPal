#include "picopal_controller.h"

#include "esp_log.h"
#include "picopal_pages.h"
#include "picopal_pico_state.h"
#include "picopal_timer.h"

static const char *TAG = "controller";

static bool timer_page_is_interactive(void)
{
    return picopal_pages_current() == PICOPAL_PAGE_TIMER &&
        !picopal_pages_overlay_active();
}

static bool handle_pico_reaction(picopal_pico_reaction_t reaction)
{
    bool accepted = picopal_pico_state_start_reaction(reaction);
    if (accepted && reaction == PICOPAL_PICO_REACTION_CODEX_DONE &&
        picopal_pages_current() != PICOPAL_PAGE_PICO) {
        picopal_pages_show_pico_overlay();
    }
    return accepted &&
        (picopal_pages_current() == PICOPAL_PAGE_PICO ||
         picopal_pages_overlay_active());
}

static bool handle_touch(picopal_event_type_t type)
{
    picopal_pico_base_t base = picopal_pico_state_base();
    if (base == PICOPAL_PICO_BASE_ERROR) {
        ESP_LOGW(TAG, "Touch ignored while Pico is in error");
        return false;
    }

    if (base == PICOPAL_PICO_BASE_SLEEP) {
        picopal_pico_state_set_base(PICOPAL_PICO_BASE_IDLE);
        picopal_pico_state_start_reaction(PICOPAL_PICO_REACTION_WAKE);
    } else if (type == PICOPAL_EVENT_TOUCH_LONG) {
        picopal_pico_state_set_base(PICOPAL_PICO_BASE_SLEEP);
    } else {
        picopal_pico_state_start_reaction(PICOPAL_PICO_REACTION_HAPPY);
    }
    return picopal_pages_current() == PICOPAL_PAGE_PICO;
}

static bool handle_motion(picopal_event_type_t type)
{
    picopal_pico_base_t base = picopal_pico_state_base();
    bool accepted = false;
    if (base == PICOPAL_PICO_BASE_ERROR) {
        ESP_LOGW(TAG, "Motion ignored while Pico is in error");
    } else if (base == PICOPAL_PICO_BASE_SLEEP) {
        picopal_pico_state_set_base(PICOPAL_PICO_BASE_IDLE);
        accepted = picopal_pico_state_start_reaction(
            PICOPAL_PICO_REACTION_WAKE
        );
    } else {
        accepted = picopal_pico_state_start_reaction(
            type == PICOPAL_EVENT_MOTION_SHAKE
                ? PICOPAL_PICO_REACTION_DIZZY
                : PICOPAL_PICO_REACTION_SURPRISED
        );
    }
    return accepted && picopal_pages_current() == PICOPAL_PAGE_PICO;
}

bool picopal_controller_handle_event(picopal_event_t event)
{
    switch (event.type) {
        case PICOPAL_EVENT_TIMER_TOGGLE:
            if (timer_page_is_interactive()) {
                picopal_timer_toggle();
                return true;
            }
            return false;

        case PICOPAL_EVENT_TIMER_RESET:
            if (timer_page_is_interactive()) {
                picopal_timer_reset();
                return true;
            }
            return false;

        case PICOPAL_EVENT_TIMER_REMOTE_TOGGLE:
            picopal_timer_toggle();
            return picopal_pages_current() == PICOPAL_PAGE_TIMER &&
                !picopal_pages_overlay_active();

        case PICOPAL_EVENT_TIMER_REMOTE_RESET:
            picopal_timer_reset();
            return picopal_pages_current() == PICOPAL_PAGE_TIMER &&
                !picopal_pages_overlay_active();

        case PICOPAL_EVENT_PICO_SET_BASE:
            picopal_pico_state_set_base((picopal_pico_base_t)event.value);
            return picopal_pages_current() == PICOPAL_PAGE_PICO;

        case PICOPAL_EVENT_PICO_REACTION:
            return handle_pico_reaction(
                (picopal_pico_reaction_t)event.value
            );

        case PICOPAL_EVENT_PICO_STATUS:
            ESP_LOGI(
                TAG,
                "Pico base=%d reaction=%d",
                picopal_pico_state_base(),
                picopal_pico_state_reaction()
            );
            return false;

        case PICOPAL_EVENT_TOUCH_SHORT:
        case PICOPAL_EVENT_TOUCH_LONG:
            return handle_touch(event.type);

        case PICOPAL_EVENT_MOTION_TAP:
        case PICOPAL_EVENT_MOTION_SHAKE:
            return handle_motion(event.type);

        default:
            return picopal_pages_handle_event(event);
    }
}
