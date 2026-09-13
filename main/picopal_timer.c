#include "picopal_timer.h"

#include <inttypes.h>
#include <stdio.h>

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "picopal_framebuffer.h"
#include "picopal_graphics.h"
#include "picopal_timer_model.h"

#define PICOPAL_MICROSECONDS_PER_SECOND 1000000ULL

static picopal_timer_model_t s_model;
static portMUX_TYPE s_timer_lock = portMUX_INITIALIZER_UNLOCKED;

void picopal_timer_init(void)
{
    picopal_timer_model_init(&s_model);
}

void picopal_timer_toggle(void)
{
    taskENTER_CRITICAL(&s_timer_lock);
    picopal_timer_model_toggle(&s_model, (uint64_t)esp_timer_get_time());
    taskEXIT_CRITICAL(&s_timer_lock);
}

void picopal_timer_reset(void)
{
    taskENTER_CRITICAL(&s_timer_lock);
    picopal_timer_model_reset(&s_model);
    taskEXIT_CRITICAL(&s_timer_lock);
}

picopal_timer_state_t picopal_timer_state(void)
{
    taskENTER_CRITICAL(&s_timer_lock);
    bool running = s_model.running;
    taskEXIT_CRITICAL(&s_timer_lock);
    return running
        ? PICOPAL_TIMER_RUNNING
        : PICOPAL_TIMER_PAUSED;
}

uint64_t picopal_timer_elapsed_seconds(void)
{
    taskENTER_CRITICAL(&s_timer_lock);
    uint64_t elapsed_us = picopal_timer_model_elapsed_us(
        &s_model,
        (uint64_t)esp_timer_get_time()
    );
    taskEXIT_CRITICAL(&s_timer_lock);
    return elapsed_us / PICOPAL_MICROSECONDS_PER_SECOND;
}

void picopal_timer_render(void)
{
    uint64_t total_seconds = picopal_timer_elapsed_seconds();
    uint64_t minutes = total_seconds / 60U;
    uint64_t seconds = total_seconds % 60U;
    char time_text[7];

    if (minutes > 999U) {
        minutes = 999U;
        seconds = 59U;
    }
    (void)snprintf(
        time_text,
        sizeof(time_text),
        "%03" PRIu64 ":%02" PRIu64,
        minutes,
        seconds
    );

    picopal_framebuffer_clear(false);
    picopal_graphics_draw_text_3x5(22, 22, time_text, 4, true);

    if (picopal_timer_state() == PICOPAL_TIMER_RUNNING) {
        picopal_graphics_fill_rect(61, 52, 6, 6, true);
    } else {
        picopal_graphics_draw_line(61, 52, 66, 52, true);
        picopal_graphics_draw_line(66, 52, 66, 57, true);
        picopal_graphics_draw_line(66, 57, 61, 57, true);
        picopal_graphics_draw_line(61, 57, 61, 52, true);
    }
}
