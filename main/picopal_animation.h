#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "picopal_pico.h"

void picopal_animation_init(void);
void picopal_animation_set_idle_enabled(bool enabled);
void picopal_animation_set_pose(picopal_pico_pose_t pose, uint32_t duration_ms);
bool picopal_animation_update(void);
void picopal_animation_render(void);
