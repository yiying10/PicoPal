#include "picopal_pages.h"

#include "picopal_framebuffer.h"
#include "picopal_graphics.h"
#include "picopal_animation.h"
#include "picopal_pico.h"
#include "picopal_timer.h"

static picopal_page_model_t s_page_model;
static bool s_pico_overlay_active;

static void render_timer_placeholder(void)
{
    picopal_timer_render();
}

static void render_draw_placeholder(void)
{
    picopal_framebuffer_clear(false);
    picopal_graphics_draw_line(0, 0, 127, 0, true);
    picopal_graphics_draw_line(127, 0, 127, 63, true);
    picopal_graphics_draw_line(127, 63, 0, 63, true);
    picopal_graphics_draw_line(0, 63, 0, 0, true);
    picopal_graphics_draw_line(28, 45, 88, 15, true);
    picopal_graphics_draw_line(31, 49, 91, 19, true);
    picopal_graphics_draw_line(28, 45, 31, 49, true);
    picopal_graphics_draw_line(88, 15, 91, 19, true);
}

void picopal_pages_init(void)
{
    picopal_page_model_init(&s_page_model);
    s_pico_overlay_active = false;
}

bool picopal_pages_handle_event(picopal_event_t event)
{
    if (s_pico_overlay_active) {
        return false;
    }

    if (event.type == PICOPAL_EVENT_PAGE_PREVIOUS) {
        return picopal_page_model_previous(&s_page_model);
    }
    if (event.type == PICOPAL_EVENT_PAGE_NEXT) {
        return picopal_page_model_next(&s_page_model);
    }
    return false;
}

void picopal_pages_render(void)
{
    if (s_pico_overlay_active) {
        picopal_animation_render();
        return;
    }

    switch (s_page_model.current) {
        case PICOPAL_PAGE_TIMER:
            render_timer_placeholder();
            break;
        case PICOPAL_PAGE_DRAW:
            render_draw_placeholder();
            break;
        case PICOPAL_PAGE_PICO:
        default:
            picopal_animation_render();
            break;
    }
}

picopal_page_t picopal_pages_current(void)
{
    return s_page_model.current;
}

void picopal_pages_show_pico_overlay(void)
{
    s_pico_overlay_active = true;
}

void picopal_pages_dismiss_overlay(void)
{
    s_pico_overlay_active = false;
}

bool picopal_pages_overlay_active(void)
{
    return s_pico_overlay_active;
}
