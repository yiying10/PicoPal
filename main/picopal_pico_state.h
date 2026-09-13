#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "picopal_pico_state_model.h"

void picopal_pico_state_init(void);
void picopal_pico_state_set_base(picopal_pico_base_t base);
bool picopal_pico_state_start_reaction(picopal_pico_reaction_t reaction);
bool picopal_pico_state_update(void);
picopal_pico_base_t picopal_pico_state_base(void);
picopal_pico_reaction_t picopal_pico_state_reaction(void);
