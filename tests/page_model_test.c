#include <assert.h>
#include <stdio.h>

#include "picopal_page_model.h"

static void boots_on_pico(void)
{
    picopal_page_model_t pages;
    picopal_page_model_init(&pages);
    assert(pages.current == PICOPAL_PAGE_PICO);
}

static void moves_between_three_pages(void)
{
    picopal_page_model_t pages;
    picopal_page_model_init(&pages);

    assert(picopal_page_model_previous(&pages));
    assert(pages.current == PICOPAL_PAGE_TIMER);
    assert(picopal_page_model_next(&pages));
    assert(pages.current == PICOPAL_PAGE_PICO);
    assert(picopal_page_model_next(&pages));
    assert(pages.current == PICOPAL_PAGE_DRAW);
}

static void does_not_wrap_at_boundaries(void)
{
    picopal_page_model_t pages;
    picopal_page_model_init(&pages);

    assert(picopal_page_model_previous(&pages));
    assert(!picopal_page_model_previous(&pages));
    assert(pages.current == PICOPAL_PAGE_TIMER);

    assert(picopal_page_model_next(&pages));
    assert(picopal_page_model_next(&pages));
    assert(!picopal_page_model_next(&pages));
    assert(pages.current == PICOPAL_PAGE_DRAW);
}

static void repeated_moves_stop_at_boundaries(void)
{
    picopal_page_model_t pages;
    picopal_page_model_init(&pages);
    for (int i = 0; i < 100; ++i) {
        picopal_page_model_next(&pages);
    }
    assert(pages.current == PICOPAL_PAGE_DRAW);

    for (int i = 0; i < 100; ++i) {
        picopal_page_model_previous(&pages);
    }
    assert(pages.current == PICOPAL_PAGE_TIMER);
}

int main(void)
{
    boots_on_pico();
    moves_between_three_pages();
    does_not_wrap_at_boundaries();
    repeated_moves_stop_at_boundaries();
    puts("page_model_test: all tests passed");
    return 0;
}
