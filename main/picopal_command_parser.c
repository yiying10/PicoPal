#include "picopal_command_parser.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "picopal_pico_state_model.h"

#define PICOPAL_COMMAND_PARSE_SIZE 64

typedef struct {
    const char *name;
    int32_t value;
} named_value_t;

static const named_value_t s_base_names[] = {
    {"idle", PICOPAL_PICO_BASE_IDLE},
    {"coding", PICOPAL_PICO_BASE_CODING},
    {"sleep", PICOPAL_PICO_BASE_SLEEP},
    {"disconnected", PICOPAL_PICO_BASE_DISCONNECTED},
    {"error", PICOPAL_PICO_BASE_ERROR},
};

static const named_value_t s_reaction_names[] = {
    {"happy", PICOPAL_PICO_REACTION_HAPPY},
    {"surprised", PICOPAL_PICO_REACTION_SURPRISED},
    {"dizzy", PICOPAL_PICO_REACTION_DIZZY},
    {"codex_done", PICOPAL_PICO_REACTION_CODEX_DONE},
    {"wake", PICOPAL_PICO_REACTION_WAKE},
};

static bool make_event(
    const named_value_t *entries,
    size_t count,
    const char *name,
    picopal_event_type_t type,
    picopal_event_t *event
)
{
    if (name == NULL) {
        return false;
    }
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(entries[i].name, name) == 0) {
            event->type = type;
            event->value = entries[i].value;
            return true;
        }
    }
    return false;
}

picopal_command_result_t picopal_command_parse(
    const char *line,
    picopal_event_t *event
)
{
    if (line == NULL || event == NULL ||
        strlen(line) >= PICOPAL_COMMAND_PARSE_SIZE) {
        return PICOPAL_COMMAND_INVALID;
    }

    char copy[PICOPAL_COMMAND_PARSE_SIZE];
    strcpy(copy, line);
    copy[strcspn(copy, "\r\n")] = '\0';

    char *save = NULL;
    char *group = strtok_r(copy, " \t:", &save);
    char *name = strtok_r(NULL, " \t:", &save);
    char *extra = strtok_r(NULL, " \t:", &save);
    if (group == NULL) {
        return PICOPAL_COMMAND_EMPTY;
    }
    if (extra != NULL) {
        return PICOPAL_COMMAND_INVALID;
    }

    if (name == NULL && make_event(
            s_base_names,
            sizeof(s_base_names) / sizeof(s_base_names[0]),
            group,
            PICOPAL_EVENT_PICO_SET_BASE,
            event
        )) {
        return PICOPAL_COMMAND_EVENT;
    }
    if (name == NULL && make_event(
            s_reaction_names,
            sizeof(s_reaction_names) / sizeof(s_reaction_names[0]),
            group,
            PICOPAL_EVENT_PICO_REACTION,
            event
        )) {
        return PICOPAL_COMMAND_EVENT;
    }
    if (strcmp(group, "status") == 0 && name == NULL) {
        event->type = PICOPAL_EVENT_PICO_STATUS;
        event->value = 0;
        return PICOPAL_COMMAND_EVENT;
    }
    if (strcmp(group, "help") == 0 && name == NULL) {
        return PICOPAL_COMMAND_HELP;
    }

    bool valid = false;
    if (strcmp(group, "base") == 0) {
        valid = make_event(
            s_base_names,
            sizeof(s_base_names) / sizeof(s_base_names[0]),
            name,
            PICOPAL_EVENT_PICO_SET_BASE,
            event
        );
    } else if (strcmp(group, "react") == 0) {
        valid = make_event(
            s_reaction_names,
            sizeof(s_reaction_names) / sizeof(s_reaction_names[0]),
            name,
            PICOPAL_EVENT_PICO_REACTION,
            event
        );
    }
    return valid ? PICOPAL_COMMAND_EVENT : PICOPAL_COMMAND_INVALID;
}
