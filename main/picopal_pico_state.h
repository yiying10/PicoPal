#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    PICOPAL_PICO_BASE_IDLE,
    PICOPAL_PICO_BASE_CODING,
    PICOPAL_PICO_BASE_SLEEP,
    PICOPAL_PICO_BASE_DISCONNECTED,
    PICOPAL_PICO_BASE_ERROR,
} picopal_pico_base_t;

typedef enum {
    PICOPAL_PICO_REACTION_NONE,
    PICOPAL_PICO_REACTION_HAPPY,
    PICOPAL_PICO_REACTION_SURPRISED,
    PICOPAL_PICO_REACTION_DIZZY,
    PICOPAL_PICO_REACTION_CODEX_DONE,
    PICOPAL_PICO_REACTION_WAKE,
} picopal_pico_reaction_t;

void picopal_pico_state_init(void);
void picopal_pico_state_set_base(picopal_pico_base_t base);
bool picopal_pico_state_start_reaction(picopal_pico_reaction_t reaction);
bool picopal_pico_state_update(void);
picopal_pico_base_t picopal_pico_state_base(void);
picopal_pico_reaction_t picopal_pico_state_reaction(void);
