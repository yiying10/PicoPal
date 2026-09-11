#include "picopal_pico_state.h"

#include <stddef.h>

#include "esp_timer.h"
#include "picopal_animation.h"

typedef struct {
    picopal_pico_pose_t pose;
    uint16_t transition_ms;
    uint16_t duration_ms;
    uint8_t priority;
} picopal_reaction_config_t;

static const picopal_reaction_config_t s_reactions[] = {
    [PICOPAL_PICO_REACTION_NONE] = {
        .pose = PICOPAL_PICO_POSE_NEUTRAL,
    },
    [PICOPAL_PICO_REACTION_HAPPY] = {
        .pose = PICOPAL_PICO_POSE_HAPPY,
        .transition_ms = 160,
        .duration_ms = 1000,
        .priority = 1,
    },
    [PICOPAL_PICO_REACTION_SURPRISED] = {
        .pose = PICOPAL_PICO_POSE_SURPRISED,
        .transition_ms = 90,
        .duration_ms = 650,
        .priority = 1,
    },
    [PICOPAL_PICO_REACTION_DIZZY] = {
        .pose = PICOPAL_PICO_POSE_DIZZY,
        .transition_ms = 100,
        .duration_ms = 1000,
        .priority = 1,
    },
    [PICOPAL_PICO_REACTION_CODEX_DONE] = {
        .pose = PICOPAL_PICO_POSE_HAPPY,
        .transition_ms = 120,
        .duration_ms = 1600,
        .priority = 3,
    },
    [PICOPAL_PICO_REACTION_WAKE] = {
        .pose = PICOPAL_PICO_POSE_SURPRISED,
        .transition_ms = 120,
        .duration_ms = 400,
        .priority = 2,
    },
};

static picopal_pico_base_t s_base;
static picopal_pico_reaction_t s_reaction;
static int64_t s_reaction_expires_us;

static picopal_pico_pose_t base_pose(picopal_pico_base_t base)
{
    switch (base) {
        case PICOPAL_PICO_BASE_CODING:
            return PICOPAL_PICO_POSE_FOCUSED;
        case PICOPAL_PICO_BASE_SLEEP:
        case PICOPAL_PICO_BASE_DISCONNECTED:
            return PICOPAL_PICO_POSE_SLEEPY;
        case PICOPAL_PICO_BASE_ERROR:
            return PICOPAL_PICO_POSE_ANGRY;
        case PICOPAL_PICO_BASE_IDLE:
        default:
            return PICOPAL_PICO_POSE_NEUTRAL;
    }
}

static void apply_base_pose(void)
{
    bool idle = s_base == PICOPAL_PICO_BASE_IDLE;
    picopal_animation_set_idle_enabled(idle);
    picopal_animation_set_pose(base_pose(s_base), 180);
}

void picopal_pico_state_init(void)
{
    picopal_animation_init();
    s_base = PICOPAL_PICO_BASE_IDLE;
    s_reaction = PICOPAL_PICO_REACTION_NONE;
    s_reaction_expires_us = 0;
}

void picopal_pico_state_set_base(picopal_pico_base_t base)
{
    if (base < PICOPAL_PICO_BASE_IDLE || base > PICOPAL_PICO_BASE_ERROR) {
        return;
    }

    s_base = base;
    if (s_reaction == PICOPAL_PICO_REACTION_NONE) {
        apply_base_pose();
    }
}

bool picopal_pico_state_start_reaction(picopal_pico_reaction_t reaction)
{
    if (reaction <= PICOPAL_PICO_REACTION_NONE ||
        reaction > PICOPAL_PICO_REACTION_WAKE) {
        return false;
    }

    const picopal_reaction_config_t *next = &s_reactions[reaction];
    const picopal_reaction_config_t *current = &s_reactions[s_reaction];
    if (s_reaction != PICOPAL_PICO_REACTION_NONE &&
        next->priority <= current->priority) {
        return false;
    }

    s_reaction = reaction;
    s_reaction_expires_us = esp_timer_get_time() +
        (int64_t)next->duration_ms * 1000;
    picopal_animation_set_idle_enabled(false);
    picopal_animation_set_pose(next->pose, next->transition_ms);
    return true;
}

bool picopal_pico_state_update(void)
{
    if (s_reaction == PICOPAL_PICO_REACTION_NONE) {
        return false;
    }
    if (esp_timer_get_time() < s_reaction_expires_us) {
        return false;
    }

    s_reaction = PICOPAL_PICO_REACTION_NONE;
    s_reaction_expires_us = 0;
    apply_base_pose();
    return true;
}

picopal_pico_base_t picopal_pico_state_base(void)
{
    return s_base;
}

picopal_pico_reaction_t picopal_pico_state_reaction(void)
{
    return s_reaction;
}
