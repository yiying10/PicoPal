#include "picopal_timer.h"

#include <inttypes.h>
#include <stdio.h>

#include "esp_timer.h"
#include "picopal_framebuffer.h"
#include "picopal_graphics.h"

#define PICOPAL_MICROSECONDS_PER_SECOND 1000000ULL

static picopal_timer_state_t s_state;
static uint64_t s_accumulated_microseconds;
static int64_t s_started_at_microseconds;

void picopal_timer_init(void)
{
    s_state = PICOPAL_TIMER_PAUSED;
    s_accumulated_microseconds = 0;
    s_started_at_microseconds = 0;
}

void picopal_timer_toggle(void)
{
    int64_t now = esp_timer_get_time();

    if (s_state == PICOPAL_TIMER_RUNNING) {
        s_accumulated_microseconds +=
            (uint64_t)(now - s_started_at_microseconds);
        s_state = PICOPAL_TIMER_PAUSED;
    } else {
        s_started_at_microseconds = now;
        s_state = PICOPAL_TIMER_RUNNING;
    }
}

void picopal_timer_reset(void)
{
    s_state = PICOPAL_TIMER_PAUSED;
    s_accumulated_microseconds = 0;
    s_started_at_microseconds = 0;
}

picopal_timer_state_t picopal_timer_state(void)
{
    return s_state;
}

uint64_t picopal_timer_elapsed_seconds(void)
{
    uint64_t total_microseconds = s_accumulated_microseconds;
    if (s_state == PICOPAL_TIMER_RUNNING) {
        int64_t now = esp_timer_get_time();
        total_microseconds += (uint64_t)(now - s_started_at_microseconds);
    }

    return total_microseconds / PICOPAL_MICROSECONDS_PER_SECOND;
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

    if (s_state == PICOPAL_TIMER_RUNNING) {
        picopal_graphics_fill_rect(61, 52, 6, 6, true);
    } else {
        picopal_graphics_draw_line(61, 52, 66, 52, true);
        picopal_graphics_draw_line(66, 52, 66, 57, true);
        picopal_graphics_draw_line(66, 57, 61, 57, true);
        picopal_graphics_draw_line(61, 57, 61, 52, true);
    }
}
