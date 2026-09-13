#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "picopal_event_types.h"

esp_err_t picopal_events_init(void);
bool picopal_events_publish(picopal_event_t event);
bool picopal_events_wait(picopal_event_t *event, uint32_t timeout_ms);
