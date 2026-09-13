#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "picopal_framebuffer.h"

#define PICOPAL_IMAGE_SIZE PICOPAL_FRAMEBUFFER_SIZE
#define PICOPAL_IMAGE_LIMIT 100

esp_err_t picopal_image_init(void);
esp_err_t picopal_image_add(
    const uint8_t *pixels,
    size_t length,
    uint32_t expected_crc,
    uint32_t *image_id
);
esp_err_t picopal_image_delete(uint32_t image_id);
esp_err_t picopal_image_select(uint32_t image_id);
bool picopal_image_select_previous(void);
bool picopal_image_select_next(void);
size_t picopal_image_count(void);
size_t picopal_image_selected_position(void);
uint32_t picopal_image_selected_id(void);
size_t picopal_image_ids(uint32_t *ids, size_t capacity);
bool picopal_image_copy_selected(uint8_t *pixels, size_t length);
bool picopal_image_available(void);
void picopal_image_render(void);
