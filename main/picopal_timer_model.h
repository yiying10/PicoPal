#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool running;
    uint64_t accumulated_us;
    uint64_t started_at_us;
} picopal_timer_model_t;

void picopal_timer_model_init(picopal_timer_model_t *model);
void picopal_timer_model_toggle(
    picopal_timer_model_t *model,
    uint64_t now_us
);
void picopal_timer_model_reset(picopal_timer_model_t *model);
uint64_t picopal_timer_model_elapsed_us(
    const picopal_timer_model_t *model,
    uint64_t now_us
);
