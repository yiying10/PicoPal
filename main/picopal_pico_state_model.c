#include "picopal_pico_state_model.h"

#include <stddef.h>

static uint8_t reaction_priority(picopal_pico_reaction_t reaction)
{
    switch (reaction) {
        case PICOPAL_PICO_REACTION_CODEX_DONE:
            return 3;
        case PICOPAL_PICO_REACTION_WAKE:
            return 2;
        case PICOPAL_PICO_REACTION_HAPPY:
        case PICOPAL_PICO_REACTION_SURPRISED:
        case PICOPAL_PICO_REACTION_DIZZY:
            return 1;
        case PICOPAL_PICO_REACTION_NONE:
        default:
            return 0;
    }
}

static uint64_t reaction_duration_us(picopal_pico_reaction_t reaction)
{
    switch (reaction) {
        case PICOPAL_PICO_REACTION_HAPPY:
        case PICOPAL_PICO_REACTION_SURPRISED:
        case PICOPAL_PICO_REACTION_DIZZY:
            return 1000000ULL;
        case PICOPAL_PICO_REACTION_CODEX_DONE:
            return 1600000ULL;
        case PICOPAL_PICO_REACTION_WAKE:
            return 400000ULL;
        case PICOPAL_PICO_REACTION_NONE:
        default:
            return 0;
    }
}

void picopal_pico_state_model_init(picopal_pico_state_model_t *model)
{
    if (model == NULL) {
        return;
    }
    model->base = PICOPAL_PICO_BASE_IDLE;
    model->reaction = PICOPAL_PICO_REACTION_NONE;
    model->reaction_expires_us = 0;
}

void picopal_pico_state_model_set_base(
    picopal_pico_state_model_t *model,
    picopal_pico_base_t base
)
{
    if (model == NULL || base < PICOPAL_PICO_BASE_IDLE ||
        base > PICOPAL_PICO_BASE_ERROR) {
        return;
    }
    model->base = base;
}

bool picopal_pico_state_model_start_reaction(
    picopal_pico_state_model_t *model,
    picopal_pico_reaction_t reaction,
    uint64_t now_us
)
{
    if (model == NULL || model->base == PICOPAL_PICO_BASE_ERROR ||
        reaction <= PICOPAL_PICO_REACTION_NONE ||
        reaction > PICOPAL_PICO_REACTION_WAKE) {
        return false;
    }
    if (model->reaction != PICOPAL_PICO_REACTION_NONE &&
        reaction_priority(reaction) <= reaction_priority(model->reaction)) {
        return false;
    }

    model->reaction = reaction;
    model->reaction_expires_us = now_us + reaction_duration_us(reaction);
    return true;
}

bool picopal_pico_state_model_update(
    picopal_pico_state_model_t *model,
    uint64_t now_us
)
{
    if (model == NULL || model->reaction == PICOPAL_PICO_REACTION_NONE ||
        now_us < model->reaction_expires_us) {
        return false;
    }

    model->reaction = PICOPAL_PICO_REACTION_NONE;
    model->reaction_expires_us = 0;
    return true;
}
