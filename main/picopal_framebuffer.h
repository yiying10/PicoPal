#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#define PICOPAL_DISPLAY_WIDTH 128
#define PICOPAL_DISPLAY_HEIGHT 64
#define PICOPAL_FRAMEBUFFER_SIZE \
    (PICOPAL_DISPLAY_WIDTH * PICOPAL_DISPLAY_HEIGHT / 8)

void picopal_framebuffer_clear(bool on);
void picopal_framebuffer_set_pixel(uint8_t x, uint8_t y, bool on);
esp_err_t picopal_framebuffer_flush(void);
