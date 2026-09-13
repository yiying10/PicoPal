#pragma once

#include <stddef.h>

#include "esp_err.h"

#define PICOPAL_STORAGE_BASE_PATH "/storage"
#define PICOPAL_IMAGES_PATH PICOPAL_STORAGE_BASE_PATH "/images"
#define PICOPAL_WEB_PATH PICOPAL_STORAGE_BASE_PATH "/web"

esp_err_t picopal_storage_init(void);
esp_err_t picopal_storage_info(size_t *total_bytes, size_t *used_bytes);
