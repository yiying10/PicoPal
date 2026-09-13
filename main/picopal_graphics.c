#include "picopal_graphics.h"

#include <stdlib.h>

#include "picopal_framebuffer.h"

static const uint8_t s_digit_rows[10][5] = {
    { 0x7, 0x5, 0x5, 0x5, 0x7 },
    { 0x2, 0x6, 0x2, 0x2, 0x7 },
    { 0x7, 0x1, 0x7, 0x4, 0x7 },
    { 0x7, 0x1, 0x7, 0x1, 0x7 },
    { 0x5, 0x5, 0x7, 0x1, 0x1 },
    { 0x7, 0x4, 0x7, 0x1, 0x7 },
    { 0x7, 0x4, 0x7, 0x5, 0x7 },
    { 0x7, 0x1, 0x1, 0x1, 0x1 },
    { 0x7, 0x5, 0x7, 0x5, 0x7 },
    { 0x7, 0x5, 0x7, 0x1, 0x7 },
};

static const uint8_t s_done_rows[5][5] = {
    { 0x6, 0x5, 0x5, 0x5, 0x6 }, /* D */
    { 0x7, 0x4, 0x6, 0x4, 0x7 }, /* E */
    { 0x5, 0x7, 0x7, 0x7, 0x5 }, /* N */
    { 0x7, 0x5, 0x5, 0x5, 0x7 }, /* O */
    { 0x2, 0x2, 0x2, 0x0, 0x2 }, /* ! */
};

static const uint8_t s_slash_rows[5] = {
    0x1,
    0x1,
    0x2,
    0x4,
    0x4,
};

static const uint8_t *text_rows(char character)
{
    if (character >= '0' && character <= '9') {
        return s_digit_rows[character - '0'];
    }
    switch (character) {
        case 'D': return s_done_rows[0];
        case 'E': return s_done_rows[1];
        case 'N': return s_done_rows[2];
        case 'O': return s_done_rows[3];
        case '!': return s_done_rows[4];
        case '/': return s_slash_rows;
        default: return NULL;
    }
}

void picopal_graphics_draw_line(
    int16_t x0,
    int16_t y0,
    int16_t x1,
    int16_t y1,
    bool on
)
{
    int16_t delta_x = abs(x1 - x0);
    int16_t step_x = x0 < x1 ? 1 : -1;
    int16_t delta_y = -abs(y1 - y0);
    int16_t step_y = y0 < y1 ? 1 : -1;
    int16_t error = delta_x + delta_y;

    while (true) {
        picopal_framebuffer_set_pixel((uint8_t)x0, (uint8_t)y0, on);
        if (x0 == x1 && y0 == y1) {
            break;
        }

        int16_t doubled_error = 2 * error;
        if (doubled_error >= delta_y) {
            error += delta_y;
            x0 += step_x;
        }
        if (doubled_error <= delta_x) {
            error += delta_x;
            y0 += step_y;
        }
    }
}

void picopal_graphics_fill_rect(
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height,
    bool on
)
{
    if (width <= 0 || height <= 0) {
        return;
    }

    for (int16_t pixel_y = y; pixel_y < y + height; ++pixel_y) {
        for (int16_t pixel_x = x; pixel_x < x + width; ++pixel_x) {
            if (pixel_x >= 0 && pixel_x < PICOPAL_DISPLAY_WIDTH &&
                pixel_y >= 0 && pixel_y < PICOPAL_DISPLAY_HEIGHT) {
                picopal_framebuffer_set_pixel(
                    (uint8_t)pixel_x,
                    (uint8_t)pixel_y,
                    on
                );
            }
        }
    }
}

void picopal_graphics_fill_rounded_rect(
    int16_t x,
    int16_t y,
    int16_t width,
    int16_t height,
    int16_t radius,
    bool on
)
{
    if (width <= 0 || height <= 0 || radius < 0) {
        return;
    }

    int16_t maximum_radius = (width < height ? width : height) / 2;
    if (radius > maximum_radius) {
        radius = maximum_radius;
    }
    if (radius == 0) {
        picopal_graphics_fill_rect(x, y, width, height, on);
        return;
    }

    int16_t corner_limit = radius - 1;
    int32_t radius_squared = (int32_t)corner_limit * corner_limit;

    for (int16_t local_y = 0; local_y < height; ++local_y) {
        for (int16_t local_x = 0; local_x < width; ++local_x) {
            bool horizontal_band = local_x >= radius && local_x < width - radius;
            bool vertical_band = local_y >= radius && local_y < height - radius;

            int16_t distance_x = local_x < radius
                ? corner_limit - local_x
                : local_x >= width - radius
                    ? local_x - (width - radius)
                    : 0;
            int16_t distance_y = local_y < radius
                ? corner_limit - local_y
                : local_y >= height - radius
                    ? local_y - (height - radius)
                    : 0;

            int32_t distance_squared =
                (int32_t)distance_x * distance_x +
                (int32_t)distance_y * distance_y;
            bool inside = horizontal_band || vertical_band || distance_squared <= radius_squared;

            if (inside) {
                int16_t pixel_x = x + local_x;
                int16_t pixel_y = y + local_y;
                if (pixel_x >= 0 && pixel_x < PICOPAL_DISPLAY_WIDTH &&
                    pixel_y >= 0 && pixel_y < PICOPAL_DISPLAY_HEIGHT) {
                    picopal_framebuffer_set_pixel(
                        (uint8_t)pixel_x,
                        (uint8_t)pixel_y,
                        on
                    );
                }
            }
        }
    }
}

void picopal_graphics_draw_text_3x5_scaled(
    int16_t x,
    int16_t y,
    const char *text,
    uint8_t scale_x,
    uint8_t scale_y,
    bool on
)
{
    if (text == NULL || scale_x == 0 || scale_y == 0) {
        return;
    }

    int16_t cursor_x = x;
    for (const char *character = text; *character != '\0'; ++character) {
        const uint8_t *rows = text_rows(*character);
        if (rows != NULL) {
            for (uint8_t row = 0; row < 5; ++row) {
                for (uint8_t column = 0; column < 3; ++column) {
                    if ((rows[row] & (1U << (2U - column))) != 0) {
                        picopal_graphics_fill_rect(
                            cursor_x + column * scale_x,
                            y + row * scale_y,
                            scale_x,
                            scale_y,
                            on
                        );
                    }
                }
            }
            cursor_x += 4 * scale_x;
        } else if (*character == ':') {
            picopal_graphics_fill_rect(
                cursor_x, y + scale_y, scale_x, scale_y, on
            );
            picopal_graphics_fill_rect(
                cursor_x, y + 3 * scale_y, scale_x, scale_y, on
            );
            cursor_x += 2 * scale_x;
        } else {
            cursor_x += 2 * scale_x;
        }
    }
}

void picopal_graphics_draw_text_3x5(
    int16_t x,
    int16_t y,
    const char *text,
    uint8_t scale,
    bool on
)
{
    picopal_graphics_draw_text_3x5_scaled(
        x, y, text, scale, scale, on
    );
}
