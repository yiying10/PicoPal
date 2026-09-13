#pragma once

#include "picopal_event_types.h"

typedef enum {
    PICOPAL_COMMAND_EMPTY,
    PICOPAL_COMMAND_EVENT,
    PICOPAL_COMMAND_HELP,
    PICOPAL_COMMAND_INVALID,
} picopal_command_result_t;

picopal_command_result_t picopal_command_parse(
    const char *line,
    picopal_event_t *event
);
