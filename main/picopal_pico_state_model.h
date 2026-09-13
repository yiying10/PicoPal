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

typedef struct {
    picopal_pico_base_t base;
    picopal_pico_reaction_t reaction;
    uint64_t reaction_expires_us;
} picopal_pico_state_model_t;

void picopal_pico_state_model_init(picopal_pico_state_model_t *model);
void picopal_pico_state_model_set_base(
    picopal_pico_state_model_t *model,
    picopal_pico_base_t base
);
bool picopal_pico_state_model_start_reaction(
    picopal_pico_state_model_t *model,
    picopal_pico_reaction_t reaction,
    uint64_t now_us
);
bool picopal_pico_state_model_update(
    picopal_pico_state_model_t *model,
    uint64_t now_us
);
