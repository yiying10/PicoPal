#include <assert.h>
#include <stdio.h>

#include "picopal_command_parser.h"
#include "picopal_pico_state_model.h"

static void accepts_direct_commands(void)
{
    picopal_event_t event;
    assert(picopal_command_parse("coding", &event) == PICOPAL_COMMAND_EVENT);
    assert(event.type == PICOPAL_EVENT_PICO_SET_BASE);
    assert(event.value == PICOPAL_PICO_BASE_CODING);

    assert(picopal_command_parse("happy", &event) == PICOPAL_COMMAND_EVENT);
    assert(event.type == PICOPAL_EVENT_PICO_REACTION);
    assert(event.value == PICOPAL_PICO_REACTION_HAPPY);

    assert(picopal_command_parse("codex_done", &event) == PICOPAL_COMMAND_EVENT);
    assert(event.type == PICOPAL_EVENT_PICO_REACTION);
    assert(event.value == PICOPAL_PICO_REACTION_CODEX_DONE);

    assert(picopal_command_parse("react:codex_done", &event) == PICOPAL_COMMAND_EVENT);
    assert(event.type == PICOPAL_EVENT_PICO_REACTION);
    assert(event.value == PICOPAL_PICO_REACTION_CODEX_DONE);
}

static void accepts_space_and_colon_formats(void)
{
    picopal_event_t event;
    assert(picopal_command_parse("react dizzy", &event) == PICOPAL_COMMAND_EVENT);
    assert(event.value == PICOPAL_PICO_REACTION_DIZZY);
    assert(picopal_command_parse("base:error", &event) == PICOPAL_COMMAND_EVENT);
    assert(event.value == PICOPAL_PICO_BASE_ERROR);
}

static void accepts_versioned_protocol(void)
{
    picopal_event_t event;
    assert(picopal_command_parse("PICO/1 base coding", &event) == PICOPAL_COMMAND_EVENT);
    assert(event.type == PICOPAL_EVENT_PICO_SET_BASE);
    assert(event.value == PICOPAL_PICO_BASE_CODING);
    assert(picopal_command_parse("PICO/1 react codex_done", &event) == PICOPAL_COMMAND_EVENT);
    assert(event.type == PICOPAL_EVENT_PICO_REACTION);
    assert(picopal_command_parse("PICO/2 base idle", &event) == PICOPAL_COMMAND_INVALID);
}

static void handles_non_event_commands(void)
{
    picopal_event_t event;
    assert(picopal_command_parse("status\n", &event) == PICOPAL_COMMAND_EVENT);
    assert(event.type == PICOPAL_EVENT_PICO_STATUS);
    assert(picopal_command_parse("help", &event) == PICOPAL_COMMAND_HELP);
    assert(picopal_command_parse("  \t", &event) == PICOPAL_COMMAND_EMPTY);
}

static void rejects_invalid_commands(void)
{
    picopal_event_t event;
    assert(picopal_command_parse("react", &event) == PICOPAL_COMMAND_INVALID);
    assert(picopal_command_parse("react unknown", &event) == PICOPAL_COMMAND_INVALID);
    assert(picopal_command_parse("base idle extra", &event) == PICOPAL_COMMAND_INVALID);
}


int main(void)
{
    accepts_direct_commands();
    accepts_space_and_colon_formats();
    accepts_versioned_protocol();
    handles_non_event_commands();
    rejects_invalid_commands();
    puts("command_parser_test: all tests passed");
    return 0;
}
