#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PICOPAL_TIMER_PAUSED,
    PICOPAL_TIMER_RUNNING,
} picopal_timer_state_t;

void picopal_timer_init(void);
void picopal_timer_toggle(void);
picopal_timer_state_t picopal_timer_state(void);
uint64_t picopal_timer_elapsed_seconds(void);
void picopal_timer_render(void);
