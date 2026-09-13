#include "picopal_pages.h"

#include <stdio.h>
#include <string.h>

#include "esp_timer.h"

#include "picopal_framebuffer.h"
#include "picopal_graphics.h"
#include "picopal_animation.h"
#include "picopal_pico.h"
#include "picopal_pico_state.h"
#include "picopal_timer.h"
#include "picopal_image.h"

#define CODEX_DONE_BLINK_US 400000LL

static picopal_page_model_t s_page_model;
static bool s_pico_overlay_active;
static int64_t s_draw_index_until_us;
static bool s_codex_done_text_visible;

static bool codex_done_text_visible(void)
{
    return picopal_pico_state_reaction() ==
            PICOPAL_PICO_REACTION_CODEX_DONE &&
        (esp_timer_get_time() / CODEX_DONE_BLINK_US) % 2 == 0;
}

static void render_pico(void)
{
    picopal_animation_render();
    if (s_codex_done_text_visible) {
        picopal_graphics_draw_text_3x5_scaled(
            7, 48, "DONE!", 6, 3, true
        );
    }
}

static void render_timer_placeholder(void)
{
    picopal_timer_render();
}

static void render_draw_placeholder(void)
{
    if (picopal_image_available()) {
        picopal_image_render();
    } else {
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

    if (s_draw_index_until_us > esp_timer_get_time()) {
        char label[12];
        snprintf(label, sizeof(label), "%u/%u",
            (unsigned)picopal_image_selected_position(),
            (unsigned)picopal_image_count());
        int width = (int)strlen(label) * 4 - 1;
        int x = 127 - width;
        picopal_graphics_fill_rect(x - 2, 56, width + 3, 8, false);
        picopal_graphics_draw_text_3x5(x, 58, label, 1, true);
    }
}

void picopal_pages_init(void)
{
    picopal_page_model_init(&s_page_model);
    s_pico_overlay_active = false;
    s_draw_index_until_us = 0;
    s_codex_done_text_visible = false;
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
    if (event.type == PICOPAL_EVENT_DRAW_IMAGE_UPDATED) {
        s_draw_index_until_us = esp_timer_get_time() + 1000000;
        return s_page_model.current == PICOPAL_PAGE_DRAW;
    }
    if (s_page_model.current == PICOPAL_PAGE_DRAW &&
        event.type == PICOPAL_EVENT_DRAW_PREVIOUS) {
        bool changed = picopal_image_select_previous();
        if (changed) s_draw_index_until_us = esp_timer_get_time() + 1000000;
        return changed;
    }
    if (s_page_model.current == PICOPAL_PAGE_DRAW &&
        event.type == PICOPAL_EVENT_DRAW_NEXT) {
        bool changed = picopal_image_select_next();
        if (changed) s_draw_index_until_us = esp_timer_get_time() + 1000000;
        return changed;
    }
    return false;
}

bool picopal_pages_update(void)
{
    bool redraw = false;
    if (s_draw_index_until_us != 0 &&
        esp_timer_get_time() >= s_draw_index_until_us) {
        s_draw_index_until_us = 0;
        redraw = s_page_model.current == PICOPAL_PAGE_DRAW;
    }

    bool text_visible = codex_done_text_visible();
    if (text_visible != s_codex_done_text_visible) {
        s_codex_done_text_visible = text_visible;
        redraw = redraw || s_page_model.current == PICOPAL_PAGE_PICO ||
            s_pico_overlay_active;
    }
    return redraw;
}

void picopal_pages_render(void)
{
    if (s_pico_overlay_active) {
        render_pico();
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
            render_pico();
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
