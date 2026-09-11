#include "picopal_events.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#define PICOPAL_EVENT_QUEUE_LENGTH 8

static QueueHandle_t s_event_queue;

esp_err_t picopal_events_init(void)
{
    s_event_queue = xQueueCreate(
        PICOPAL_EVENT_QUEUE_LENGTH,
        sizeof(picopal_event_t)
    );
    return s_event_queue == NULL ? ESP_ERR_NO_MEM : ESP_OK;
}

bool picopal_events_publish(picopal_event_t event)
{
    if (s_event_queue == NULL) {
        return false;
    }
    return xQueueSend(s_event_queue, &event, 0) == pdTRUE;
}

bool picopal_events_wait(picopal_event_t *event, uint32_t timeout_ms)
{
    if (s_event_queue == NULL || event == NULL) {
        return false;
    }
    return xQueueReceive(
        s_event_queue,
        event,
        pdMS_TO_TICKS(timeout_ms)
    ) == pdTRUE;
}
