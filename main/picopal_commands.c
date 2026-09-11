#include "picopal_commands.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "picopal_events.h"
#include "picopal_pico_state.h"

#define PICOPAL_COMMAND_LINE_SIZE 64

typedef struct {
    const char *name;
    int32_t value;
} picopal_named_value_t;

static const char *TAG = "commands";

static const picopal_named_value_t s_base_names[] = {
    { "idle", PICOPAL_PICO_BASE_IDLE },
    { "coding", PICOPAL_PICO_BASE_CODING },
    { "sleep", PICOPAL_PICO_BASE_SLEEP },
    { "disconnected", PICOPAL_PICO_BASE_DISCONNECTED },
    { "error", PICOPAL_PICO_BASE_ERROR },
};

static const picopal_named_value_t s_reaction_names[] = {
    { "happy", PICOPAL_PICO_REACTION_HAPPY },
    { "surprised", PICOPAL_PICO_REACTION_SURPRISED },
    { "dizzy", PICOPAL_PICO_REACTION_DIZZY },
    { "codex_done", PICOPAL_PICO_REACTION_CODEX_DONE },
    { "wake", PICOPAL_PICO_REACTION_WAKE },
};

static bool find_named_value(
    const picopal_named_value_t *entries,
    size_t entry_count,
    const char *name,
    int32_t *value
)
{
    if (name == NULL || value == NULL) {
        return false;
    }

    for (size_t i = 0; i < entry_count; ++i) {
        if (strcmp(entries[i].name, name) == 0) {
            *value = entries[i].value;
            return true;
        }
    }
    return false;
}

static void publish_command_event(picopal_event_type_t type, int32_t value)
{
    picopal_event_t event = {
        .type = type,
        .value = value,
    };
    if (!picopal_events_publish(event)) {
        ESP_LOGW(TAG, "Event queue full; command discarded");
    }
}

static void print_help(void)
{
    ESP_LOGI(TAG, "Base: idle|coding|sleep|disconnected|error");
    ESP_LOGI(TAG, "React: happy|surprised|dizzy|codex_done|wake");
    ESP_LOGI(TAG, "Also accepted: base:coding and react:happy");
    ESP_LOGI(TAG, "status");
}

static void handle_line(char *line)
{
    line[strcspn(line, "\r\n")] = '\0';

    char *save_pointer = NULL;
    char *group = strtok_r(line, " \t:", &save_pointer);
    char *name = strtok_r(NULL, " \t:", &save_pointer);
    if (group == NULL) {
        return;
    }

    int32_t direct_value = 0;
    if (name == NULL && find_named_value(
            s_base_names,
            sizeof(s_base_names) / sizeof(s_base_names[0]),
            group,
            &direct_value)) {
        publish_command_event(PICOPAL_EVENT_PICO_SET_BASE, direct_value);
        return;
    }
    if (name == NULL && find_named_value(
            s_reaction_names,
            sizeof(s_reaction_names) / sizeof(s_reaction_names[0]),
            group,
            &direct_value)) {
        publish_command_event(PICOPAL_EVENT_PICO_REACTION, direct_value);
        return;
    }

    const picopal_named_value_t *entries = NULL;
    size_t entry_count = 0;
    picopal_event_type_t event_type = PICOPAL_EVENT_PICO_STATUS;

    if (strcmp(group, "base") == 0) {
        entries = s_base_names;
        entry_count = sizeof(s_base_names) / sizeof(s_base_names[0]);
        event_type = PICOPAL_EVENT_PICO_SET_BASE;
    } else if (strcmp(group, "react") == 0) {
        entries = s_reaction_names;
        entry_count = sizeof(s_reaction_names) / sizeof(s_reaction_names[0]);
        event_type = PICOPAL_EVENT_PICO_REACTION;
    } else if (strcmp(group, "status") == 0) {
        publish_command_event(PICOPAL_EVENT_PICO_STATUS, 0);
        return;
    } else if (strcmp(group, "help") == 0) {
        print_help();
        return;
    } else {
        ESP_LOGW(TAG, "Unknown command: %s", group);
        print_help();
        return;
    }

    int32_t value = 0;
    if (!find_named_value(entries, entry_count, name, &value)) {
        ESP_LOGW(TAG, "Unknown or missing command value");
        print_help();
        return;
    }
    publish_command_event(event_type, value);
}

static void command_task(void *context)
{
    (void)context;
    char line[PICOPAL_COMMAND_LINE_SIZE];
    size_t line_length = 0;
    bool discarding_overflow = false;

    print_help();
    while (true) {
        int input = fgetc(stdin);
        if (input == EOF) {
            clearerr(stdin);
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        char character = (char)input;
        if (character == '\r' || character == '\n') {
            if (discarding_overflow) {
                discarding_overflow = false;
                line_length = 0;
                continue;
            }
            if (line_length > 0) {
                line[line_length] = '\0';
                handle_line(line);
                line_length = 0;
            }
        } else if (character == '\b' || character == 0x7F) {
            if (!discarding_overflow && line_length > 0) {
                --line_length;
            }
        } else if (!discarding_overflow) {
            if (line_length < sizeof(line) - 1) {
                line[line_length++] = character;
            } else {
                ESP_LOGW(TAG, "Command too long; discarded");
                discarding_overflow = true;
                line_length = 0;
            }
        }
    }
}

esp_err_t picopal_commands_init(void)
{
    BaseType_t created = xTaskCreate(
        command_task,
        "picopal_commands",
        3072,
        NULL,
        4,
        NULL
    );
    return created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
