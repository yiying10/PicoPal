#include "picopal_animation.h"

#include <stddef.h>

#include "esp_timer.h"

typedef struct {
    picopal_pico_pose_t pose;
    uint32_t transition_ms;
    uint32_t hold_ms;
} picopal_idle_step_t;

static const picopal_idle_step_t s_idle_sequence[] = {
    { PICOPAL_PICO_POSE_LOOK_LEFT, 220, 650 },
    { PICOPAL_PICO_POSE_NEUTRAL, 220, 900 },
    { PICOPAL_PICO_POSE_LOOK_UP, 220, 500 },
    { PICOPAL_PICO_POSE_NEUTRAL, 220, 1200 },
    { PICOPAL_PICO_POSE_SLEEPY, 90, 120 },
    { PICOPAL_PICO_POSE_NEUTRAL, 110, 1400 },
    { PICOPAL_PICO_POSE_LOOK_RIGHT, 220, 650 },
    { PICOPAL_PICO_POSE_NEUTRAL, 220, 800 },
    { PICOPAL_PICO_POSE_LOOK_DOWN, 220, 500 },
    { PICOPAL_PICO_POSE_NEUTRAL, 220, 1600 },
};

static picopal_pico_geometry_t s_current;
static picopal_pico_geometry_t s_start;
static picopal_pico_geometry_t s_target;
static int64_t s_transition_started_us;
static uint32_t s_transition_duration_ms;
static int64_t s_next_idle_step_us;
static size_t s_idle_index;
static bool s_transition_active;

static int16_t interpolate_value(int16_t from, int16_t to, float progress)
{
    return (int16_t)(from + (to - from) * progress);
}

static picopal_eye_geometry_t interpolate_eye(
    const picopal_eye_geometry_t *from,
    const picopal_eye_geometry_t *to,
    float progress
)
{
    return (picopal_eye_geometry_t){
        .x = interpolate_value(from->x, to->x, progress),
        .y = interpolate_value(from->y, to->y, progress),
        .width = interpolate_value(from->width, to->width, progress),
        .height = interpolate_value(from->height, to->height, progress),
        .radius = interpolate_value(from->radius, to->radius, progress),
    };
}

void picopal_animation_init(void)
{
    s_current = *picopal_pico_pose_geometry(PICOPAL_PICO_POSE_NEUTRAL);
    s_start = s_current;
    s_target = s_current;
    s_transition_started_us = 0;
    s_transition_duration_ms = 0;
    s_idle_index = 0;
    s_transition_active = false;
    s_next_idle_step_us = esp_timer_get_time() + 1200000;
}

void picopal_animation_set_pose(picopal_pico_pose_t pose, uint32_t duration_ms)
{
    s_start = s_current;
    s_target = *picopal_pico_pose_geometry(pose);
    s_transition_started_us = esp_timer_get_time();
    s_transition_duration_ms = duration_ms;
    s_transition_active = duration_ms > 0;

    if (!s_transition_active) {
        s_current = s_target;
    }
}

bool picopal_animation_update(void)
{
    int64_t now = esp_timer_get_time();

    if (!s_transition_active && now >= s_next_idle_step_us) {
        const picopal_idle_step_t *step = &s_idle_sequence[s_idle_index];
        picopal_animation_set_pose(step->pose, step->transition_ms);
        s_next_idle_step_us = now +
            (int64_t)(step->transition_ms + step->hold_ms) * 1000;
        s_idle_index = (s_idle_index + 1) %
            (sizeof(s_idle_sequence) / sizeof(s_idle_sequence[0]));
    }

    if (!s_transition_active) {
        return false;
    }

    int64_t elapsed_us = now - s_transition_started_us;
    float progress = (float)elapsed_us /
        (float)((int64_t)s_transition_duration_ms * 1000);
    if (progress >= 1.0f) {
        s_current = s_target;
        s_transition_active = false;
        return true;
    }

    float eased_progress = progress * progress * (3.0f - 2.0f * progress);
    s_current.left = interpolate_eye(
        &s_start.left,
        &s_target.left,
        eased_progress
    );
    s_current.right = interpolate_eye(
        &s_start.right,
        &s_target.right,
        eased_progress
    );
    s_current.style = progress < 0.5f ? s_start.style : s_target.style;
    return true;
}

void picopal_animation_render(void)
{
    picopal_pico_render_geometry(&s_current);
}
