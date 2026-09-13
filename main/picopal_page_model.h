#pragma once

#include <stdbool.h>

typedef enum {
    PICOPAL_PAGE_TIMER,
    PICOPAL_PAGE_PICO,
    PICOPAL_PAGE_DRAW,
} picopal_page_t;

typedef struct {
    picopal_page_t current;
} picopal_page_model_t;

void picopal_page_model_init(picopal_page_model_t *model);
bool picopal_page_model_previous(picopal_page_model_t *model);
bool picopal_page_model_next(picopal_page_model_t *model);
