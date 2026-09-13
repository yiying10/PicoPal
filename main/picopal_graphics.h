#pragma once

#include <stdbool.h>
#include <stdint.h>

void picopal_graphics_draw_line(
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1,
    bool on
);

void picopal_graphics_fill_rect(
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height,
    bool on
);

void picopal_graphics_fill_rounded_rect(
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height,
    int16_t radius,
    bool on
);

void picopal_graphics_draw_text_3x5(
    int16_t x,
    int16_t y,
    const char *text,
    uint8_t scale,
    bool on
);

void picopal_graphics_draw_text_3x5_scaled(
    int16_t x,
    int16_t y,
    const char *text,
    uint8_t scale_x,
    uint8_t scale_y,
    bool on
);
