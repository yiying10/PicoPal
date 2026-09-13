#include "picopal_framebuffer.h"

#include <stddef.h>
#include <string.h>

#include "esp_check.h"
#include "picopal_display.h"

static uint8_t s_pixels[PICOPAL_FRAMEBUFFER_SIZE];

void picopal_framebuffer_clear(bool on)
{
    memset(s_pixels, on ? 0xFF : 0x00, sizeof(s_pixels));
}

void picopal_framebuffer_set_pixel(uint8_t x, uint8_t y, bool on)
{
    if (x >= PICOPAL_DISPLAY_WIDTH || y >= PICOPAL_DISPLAY_HEIGHT) {
        return;
    }

    size_t byte_index = ((size_t)y / 8U) * PICOPAL_DISPLAY_WIDTH + x;
    uint8_t bit_mask = (uint8_t)(1U << (y % 8U));

    if (on) {
        s_pixels[byte_index] |= bit_mask;
    } else {
        s_pixels[byte_index] &= (uint8_t)~bit_mask;
    }
}

void picopal_framebuffer_copy(const uint8_t *pixels, size_t length)
{
    if (pixels == NULL || length != sizeof(s_pixels)) {
        return;
    }
    memcpy(s_pixels, pixels, sizeof(s_pixels));
}

esp_err_t picopal_framebuffer_flush(void)
{
    ESP_RETURN_ON_ERROR(picopal_display_send_command(0x21),
                        "framebuffer", "column command failed");
    ESP_RETURN_ON_ERROR(picopal_display_send_command(0x00),
                        "framebuffer", "column start failed");
    ESP_RETURN_ON_ERROR(picopal_display_send_command(0x7F),
                        "framebuffer", "column end failed");
    ESP_RETURN_ON_ERROR(picopal_display_send_command(0x22),
                        "framebuffer", "page command failed");
    ESP_RETURN_ON_ERROR(picopal_display_send_command(0x00),
                        "framebuffer", "page start failed");
    ESP_RETURN_ON_ERROR(picopal_display_send_command(0x07),
                        "framebuffer", "page end failed");

    return picopal_display_send_data(s_pixels, sizeof(s_pixels));
}
