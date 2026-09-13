#include "picopal_page_model.h"

#include <stddef.h>

void picopal_page_model_init(picopal_page_model_t *model)
{
    if (model != NULL) {
        model->current = PICOPAL_PAGE_PICO;
    }
}

bool picopal_page_model_previous(picopal_page_model_t *model)
{
    if (model == NULL || model->current <= PICOPAL_PAGE_TIMER) {
        return false;
    }
    --model->current;
    return true;
}

bool picopal_page_model_next(picopal_page_model_t *model)
{
    if (model == NULL || model->current >= PICOPAL_PAGE_DRAW) {
        return false;
    }
    ++model->current;
    return true;
}
