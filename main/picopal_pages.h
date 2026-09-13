#pragma once

#include <stdbool.h>

#include "picopal_events.h"
#include "picopal_page_model.h"

void picopal_pages_init(void);
bool picopal_pages_handle_event(picopal_event_t event);
bool picopal_pages_update(void);
void picopal_pages_render(void);
picopal_page_t picopal_pages_current(void);
void picopal_pages_show_pico_overlay(void);
void picopal_pages_dismiss_overlay(void);
bool picopal_pages_overlay_active(void);
