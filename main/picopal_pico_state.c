#include "picopal_pico_state.h"

#include <stddef.h>

#include "esp_timer.h"
#include "picopal_animation.h"

typedef struct {
    picopal_pico_pose_t pose;
    uint16_t transition_ms;
} picopal_reaction_config_t;

static const picopal_reaction_config_t s_reactions[] = {
    [PICOPAL_PICO_REACTION_NONE] = {
        .pose = PICOPAL_PICO_POSE_NEUTRAL,
    },
    [PICOPAL_PICO_REACTION_HAPPY] = {
        .pose = PICOPAL_PICO_POSE_HAPPY,
        .transition_ms = 160,
    },
    [PICOPAL_PICO_REACTION_SURPRISED] = {
        .pose = PICOPAL_PICO_POSE_SURPRISED,
        .transition_ms = 90,
    },
    [PICOPAL_PICO_REACTION_DIZZY] = {
        .pose = PICOPAL_PICO_POSE_DIZZY,
        .transition_ms = 100,
    },
    [PICOPAL_PICO_REACTION_CODEX_DONE] = {
        .pose = PICOPAL_PICO_POSE_HAPPY,
        .transition_ms = 120,
    },
    [PICOPAL_PICO_REACTION_WAKE] = {
        .pose = PICOPAL_PICO_POSE_SURPRISED,
        .transition_ms = 120,
    },
};

static picopal_pico_state_model_t s_model;

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
    bool idle = s_model.base == PICOPAL_PICO_BASE_IDLE;
    picopal_animation_set_idle_enabled(idle);
    picopal_animation_set_pose(base_pose(s_model.base), 180);
}

void picopal_pico_state_init(void)
{
    picopal_animation_init();
    picopal_pico_state_model_init(&s_model);
}

void picopal_pico_state_set_base(picopal_pico_base_t base)
{
    picopal_pico_state_model_set_base(&s_model, base);
    if (s_model.reaction == PICOPAL_PICO_REACTION_NONE) {
        apply_base_pose();
    }
}

bool picopal_pico_state_start_reaction(picopal_pico_reaction_t reaction)
{
    if (!picopal_pico_state_model_start_reaction(
            &s_model,
            reaction,
            (uint64_t)esp_timer_get_time()
        )) {
        return false;
    }

    const picopal_reaction_config_t *next = &s_reactions[reaction];
    picopal_animation_set_idle_enabled(false);
    picopal_animation_set_pose(next->pose, next->transition_ms);
    return true;
}

bool picopal_pico_state_update(void)
{
    if (!picopal_pico_state_model_update(
            &s_model,
            (uint64_t)esp_timer_get_time()
        )) {
        return false;
    }
    apply_base_pose();
    return true;
}

picopal_pico_base_t picopal_pico_state_base(void)
{
    return s_model.base;
}

picopal_pico_reaction_t picopal_pico_state_reaction(void)
{
    return s_model.reaction;
}
