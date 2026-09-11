#pragma once

#include <stdbool.h>

#include "picopal_events.h"

typedef enum {
    PICOPAL_PAGE_TIMER,
    PICOPAL_PAGE_PICO,
    PICOPAL_PAGE_DRAW,
} picopal_page_t;

void picopal_pages_init(void);
bool picopal_pages_handle_event(picopal_event_t event);
void picopal_pages_render(void);
picopal_page_t picopal_pages_current(void);
void picopal_pages_show_pico_overlay(void);
void picopal_pages_dismiss_overlay(void);
bool picopal_pages_overlay_active(void);
