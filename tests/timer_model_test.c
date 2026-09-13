#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "picopal_timer_model.h"

#define SECOND_US 1000000ULL

static void starts_paused_at_zero(void)
{
    picopal_timer_model_t timer;
    picopal_timer_model_init(&timer);

    assert(!timer.running);
    assert(picopal_timer_model_elapsed_us(&timer, 20 * SECOND_US) == 0);
}

static void accumulates_only_while_running(void)
{
    picopal_timer_model_t timer;
    picopal_timer_model_init(&timer);

    picopal_timer_model_toggle(&timer, 10 * SECOND_US);
    assert(timer.running);
    assert(
        picopal_timer_model_elapsed_us(&timer, 25 * SECOND_US) ==
        15 * SECOND_US
    );

    picopal_timer_model_toggle(&timer, 25 * SECOND_US);
    assert(!timer.running);
    assert(
        picopal_timer_model_elapsed_us(&timer, 40 * SECOND_US) ==
        15 * SECOND_US
    );
}

static void resume_adds_another_segment(void)
{
    picopal_timer_model_t timer;
    picopal_timer_model_init(&timer);

    picopal_timer_model_toggle(&timer, 0);
    picopal_timer_model_toggle(&timer, 12 * SECOND_US);
    picopal_timer_model_toggle(&timer, 30 * SECOND_US);

    assert(
        picopal_timer_model_elapsed_us(&timer, 38 * SECOND_US) ==
        20 * SECOND_US
    );
}

static void reset_stops_and_clears(void)
{
    picopal_timer_model_t timer;
    picopal_timer_model_init(&timer);
    picopal_timer_model_toggle(&timer, 2 * SECOND_US);

    picopal_timer_model_reset(&timer);

    assert(!timer.running);
    assert(picopal_timer_model_elapsed_us(&timer, 50 * SECOND_US) == 0);
}

static void reset_and_start_from_beginning(void)
{
    picopal_timer_model_t timer;
    picopal_timer_model_init(&timer);
    picopal_timer_model_toggle(&timer, 0);
    picopal_timer_model_toggle(&timer, 1000 * SECOND_US);

    picopal_timer_model_reset(&timer);
    picopal_timer_model_toggle(&timer, 2000 * SECOND_US);

    assert(
        picopal_timer_model_elapsed_us(&timer, 2005 * SECOND_US) ==
        5 * SECOND_US
    );
}

int main(void)
{
    starts_paused_at_zero();
    accumulates_only_while_running();
    resume_adds_another_segment();
    reset_stops_and_clears();
    reset_and_start_from_beginning();
    puts("timer_model_test: all tests passed");
    return 0;
}
