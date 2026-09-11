#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

typedef enum {
    PICOPAL_EVENT_PAGE_PREVIOUS,
    PICOPAL_EVENT_PAGE_NEXT,
    PICOPAL_EVENT_DRAW_PREVIOUS,
    PICOPAL_EVENT_DRAW_NEXT,
    PICOPAL_EVENT_TIMER_TOGGLE,
    PICOPAL_EVENT_PICO_SET_BASE,
    PICOPAL_EVENT_PICO_REACTION,
    PICOPAL_EVENT_PICO_STATUS,
} picopal_event_type_t;

typedef struct {
    picopal_event_type_t type;
    int32_t value;
} picopal_event_t;

esp_err_t picopal_events_init(void);
bool picopal_events_publish(picopal_event_t event);
bool picopal_events_wait(picopal_event_t *event, uint32_t timeout_ms);
