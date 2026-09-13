#include "picopal_timer_model.h"

#include <stddef.h>

void picopal_timer_model_init(picopal_timer_model_t *model)
{
    if (model == NULL) {
        return;
    }
    model->running = false;
    model->accumulated_us = 0;
    model->started_at_us = 0;
}

void picopal_timer_model_toggle(
    picopal_timer_model_t *model,
    uint64_t now_us
)
{
    if (model == NULL) {
        return;
    }

    if (model->running) {
        if (now_us >= model->started_at_us) {
            model->accumulated_us += now_us - model->started_at_us;
        }
        model->running = false;
    } else {
        model->started_at_us = now_us;
        model->running = true;
    }
}

void picopal_timer_model_reset(picopal_timer_model_t *model)
{
    picopal_timer_model_init(model);
}

uint64_t picopal_timer_model_elapsed_us(
    const picopal_timer_model_t *model,
    uint64_t now_us
)
{
    if (model == NULL) {
        return 0;
    }

    uint64_t elapsed_us = model->accumulated_us;
    if (model->running && now_us >= model->started_at_us) {
        elapsed_us += now_us - model->started_at_us;
    }
    return elapsed_us;
}
