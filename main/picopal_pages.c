#include "picopal_pages.h"

#include "picopal_framebuffer.h"
#include "picopal_graphics.h"
#include "picopal_animation.h"
#include "picopal_pico.h"
#include "picopal_timer.h"

static picopal_page_t s_current_page;
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
    s_current_page = PICOPAL_PAGE_PICO;
    s_pico_overlay_active = false;
}

bool picopal_pages_handle_event(picopal_event_t event)
{
    if (s_pico_overlay_active) {
        return false;
    }

    picopal_page_t previous_page = s_current_page;

    if (event.type == PICOPAL_EVENT_PAGE_PREVIOUS &&
        s_current_page > PICOPAL_PAGE_TIMER) {
        --s_current_page;
    } else if (event.type == PICOPAL_EVENT_PAGE_NEXT &&
               s_current_page < PICOPAL_PAGE_DRAW) {
        ++s_current_page;
    }

    return previous_page != s_current_page;
}

void picopal_pages_render(void)
{
    if (s_pico_overlay_active) {
        picopal_animation_render();
        return;
    }

    switch (s_current_page) {
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
    return s_current_page;
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
