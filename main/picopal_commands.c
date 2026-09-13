#include "picopal_commands.h"

#include <stdbool.h>
#include <stdio.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "picopal_command_parser.h"
#include "picopal_events.h"

#define PICOPAL_COMMAND_LINE_SIZE 64

static const char *TAG = "commands";

static void print_help(void)
{
    ESP_LOGI(TAG, "Base: idle|coding|sleep|disconnected|error");
    ESP_LOGI(TAG, "React: happy|surprised|dizzy|codex_done|wake");
    ESP_LOGI(TAG, "Also accepted: base:coding and react:happy");
    ESP_LOGI(TAG, "status");
}

static void handle_line(const char *line)
{
    picopal_event_t event;
    picopal_command_result_t result = picopal_command_parse(line, &event);
    if (result == PICOPAL_COMMAND_EVENT) {
        if (!picopal_events_publish(event)) {
            ESP_LOGW(TAG, "Event queue full; command discarded");
        }
    } else if (result == PICOPAL_COMMAND_HELP) {
        print_help();
    } else if (result == PICOPAL_COMMAND_INVALID) {
        ESP_LOGW(TAG, "Unknown or invalid command: %s", line);
        print_help();
    }
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
