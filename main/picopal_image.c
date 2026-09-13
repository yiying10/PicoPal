#include "picopal_image.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "picopal_crc32.h"
#include "picopal_storage.h"

#define IMAGE_INDEX_MAGIC 0x5049434FU
#define IMAGE_INDEX_VERSION 1U
#define IMAGE_INDEX_PATH PICOPAL_IMAGES_PATH "/index.bin"
#define IMAGE_INDEX_TEMP_PATH PICOPAL_IMAGES_PATH "/index.tmp"
#define IMAGE_UPLOAD_TEMP_PATH PICOPAL_IMAGES_PATH "/upload.tmp"
#define LEGACY_IMAGE_PATH PICOPAL_IMAGES_PATH "/current.bin"

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t count;
    uint32_t selected_id;
    uint32_t next_id;
    uint32_t ids[PICOPAL_IMAGE_LIMIT];
} image_index_t;

static const char *TAG = "image";
static image_index_t s_index;
static uint8_t s_pixels[PICOPAL_IMAGE_SIZE];
/* Large operation buffers live outside task stacks and are mutex-protected. */
static image_index_t s_work_index;
static uint8_t s_work_pixels[PICOPAL_IMAGE_SIZE];
static SemaphoreHandle_t s_mutex;

static void image_path(uint32_t id, char *path, size_t path_size)
{
    snprintf(path, path_size, PICOPAL_IMAGES_PATH "/%08lu.bin", (unsigned long)id);
}

static bool write_file(const char *path, const uint8_t *data, size_t length)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL) {
        return false;
    }
    bool written = fwrite(data, 1, length, file) == length &&
        fflush(file) == 0 && fsync(fileno(file)) == 0;
    bool closed = fclose(file) == 0;
    return written && closed;
}

static esp_err_t save_index(const image_index_t *index)
{
    if (!write_file(
            IMAGE_INDEX_TEMP_PATH,
            (const uint8_t *)index,
            sizeof(*index)
        )) {
        unlink(IMAGE_INDEX_TEMP_PATH);
        return ESP_FAIL;
    }
    if (rename(IMAGE_INDEX_TEMP_PATH, IMAGE_INDEX_PATH) != 0) {
        unlink(IMAGE_INDEX_TEMP_PATH);
        return ESP_FAIL;
    }
    return ESP_OK;
}

static bool read_pixels(uint32_t id, uint8_t *pixels)
{
    char path[64];
    image_path(id, path, sizeof(path));
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return false;
    }
    size_t bytes_read = fread(pixels, 1, PICOPAL_IMAGE_SIZE, file);
    int extra = fgetc(file);
    fclose(file);
    return bytes_read == PICOPAL_IMAGE_SIZE && extra == EOF;
}

static int selected_index(const image_index_t *index)
{
    for (uint16_t i = 0; i < index->count; ++i) {
        if (index->ids[i] == index->selected_id) {
            return i;
        }
    }
    return -1;
}

static bool index_is_valid(const image_index_t *index)
{
    return index->magic == IMAGE_INDEX_MAGIC &&
        index->version == IMAGE_INDEX_VERSION &&
        index->count <= PICOPAL_IMAGE_LIMIT &&
        index->next_id > 0 &&
        (index->count == 0 || selected_index(index) >= 0);
}

static void empty_index(image_index_t *index)
{
    memset(index, 0, sizeof(*index));
    index->magic = IMAGE_INDEX_MAGIC;
    index->version = IMAGE_INDEX_VERSION;
    index->next_id = 1;
}

static void load_index(void)
{
    FILE *file = fopen(IMAGE_INDEX_PATH, "rb");
    if (file != NULL) {
        size_t bytes_read = fread(&s_index, 1, sizeof(s_index), file);
        int extra = fgetc(file);
        fclose(file);
        if (bytes_read == sizeof(s_index) && extra == EOF &&
            index_is_valid(&s_index) &&
            (s_index.count == 0 || read_pixels(s_index.selected_id, s_pixels))) {
            return;
        }
        ESP_LOGW(TAG, "Image index invalid; starting an empty gallery");
    }
    empty_index(&s_index);
    memset(s_pixels, 0, sizeof(s_pixels));
}

static void migrate_legacy_image(void)
{
    if (s_index.count != 0 || access(LEGACY_IMAGE_PATH, F_OK) != 0) {
        return;
    }
    FILE *file = fopen(LEGACY_IMAGE_PATH, "rb");
    if (file == NULL ||
        fread(s_work_pixels, 1, sizeof(s_work_pixels), file) !=
            sizeof(s_work_pixels) ||
        fgetc(file) != EOF) {
        if (file != NULL) {
            fclose(file);
        }
        return;
    }
    fclose(file);

    char path[64];
    image_path(1, path, sizeof(path));
    if (rename(LEGACY_IMAGE_PATH, path) == 0) {
        s_index.ids[0] = 1;
        s_index.count = 1;
        s_index.selected_id = 1;
        s_index.next_id = 2;
        memcpy(s_pixels, s_work_pixels, sizeof(s_pixels));
        if (save_index(&s_index) == ESP_OK) {
            ESP_LOGI(TAG, "Previous drawing migrated into gallery");
        }
    }
}

esp_err_t picopal_image_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    if (s_mutex == NULL) {
        return ESP_ERR_NO_MEM;
    }
    load_index();
    migrate_legacy_image();
    ESP_LOGI(TAG, "Gallery ready: %u image(s)", s_index.count);
    return ESP_OK;
}

esp_err_t picopal_image_add(
    const uint8_t *pixels,
    size_t length,
    uint32_t expected_crc,
    uint32_t *image_id
)
{
    if (pixels == NULL || length != PICOPAL_IMAGE_SIZE) {
        return ESP_ERR_INVALID_SIZE;
    }
    if (picopal_crc32(pixels, length) != expected_crc) {
        return ESP_ERR_INVALID_CRC;
    }

    size_t total_bytes = 0;
    size_t used_bytes = 0;
    if (picopal_storage_info(&total_bytes, &used_bytes) != ESP_OK ||
        total_bytes < used_bytes ||
        total_bytes - used_bytes < PICOPAL_IMAGE_SIZE + 8192) {
        return ESP_ERR_NO_MEM;
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);
    if (s_index.count >= PICOPAL_IMAGE_LIMIT) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NO_MEM;
    }

    uint32_t id = s_index.next_id;
    char final_path[64];
    image_path(id, final_path, sizeof(final_path));
    if (!write_file(IMAGE_UPLOAD_TEMP_PATH, pixels, length) ||
        rename(IMAGE_UPLOAD_TEMP_PATH, final_path) != 0) {
        unlink(IMAGE_UPLOAD_TEMP_PATH);
        xSemaphoreGive(s_mutex);
        return ESP_FAIL;
    }

    s_work_index = s_index;
    s_work_index.ids[s_work_index.count++] = id;
    s_work_index.selected_id = id;
    s_work_index.next_id = id + 1;
    if (save_index(&s_work_index) != ESP_OK) {
        unlink(final_path);
        xSemaphoreGive(s_mutex);
        return ESP_FAIL;
    }

    s_index = s_work_index;
    memcpy(s_pixels, pixels, sizeof(s_pixels));
    if (image_id != NULL) {
        *image_id = id;
    }
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

esp_err_t picopal_image_select(uint32_t image_id)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_work_index = s_index;
    s_work_index.selected_id = image_id;
    if (selected_index(&s_work_index) < 0) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }
    if (!read_pixels(image_id, s_work_pixels) ||
        save_index(&s_work_index) != ESP_OK) {
        xSemaphoreGive(s_mutex);
        return ESP_FAIL;
    }
    s_index = s_work_index;
    memcpy(s_pixels, s_work_pixels, sizeof(s_pixels));
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

static bool select_offset(int offset)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    int current = selected_index(&s_index);
    int target = current + offset;
    if (current < 0 || target < 0 || target >= s_index.count) {
        xSemaphoreGive(s_mutex);
        return false;
    }
    uint32_t target_id = s_index.ids[target];
    xSemaphoreGive(s_mutex);
    return picopal_image_select(target_id) == ESP_OK;
}

bool picopal_image_select_previous(void)
{
    return select_offset(-1);
}

bool picopal_image_select_next(void)
{
    return select_offset(1);
}

esp_err_t picopal_image_delete(uint32_t image_id)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    int removed = -1;
    for (uint16_t i = 0; i < s_index.count; ++i) {
        if (s_index.ids[i] == image_id) {
            removed = i;
            break;
        }
    }
    if (removed < 0) {
        xSemaphoreGive(s_mutex);
        return ESP_ERR_NOT_FOUND;
    }

    s_work_index = s_index;
    for (uint16_t i = (uint16_t)removed; i + 1 < s_work_index.count; ++i) {
        s_work_index.ids[i] = s_work_index.ids[i + 1];
    }
    --s_work_index.count;
    memset(s_work_pixels, 0, sizeof(s_work_pixels));
    if (image_id == s_work_index.selected_id) {
        if (s_work_index.count == 0) {
            s_work_index.selected_id = 0;
        } else {
            uint16_t replacement = removed < s_work_index.count
                ? (uint16_t)removed
                : s_work_index.count - 1;
            s_work_index.selected_id = s_work_index.ids[replacement];
            if (!read_pixels(s_work_index.selected_id, s_work_pixels)) {
                xSemaphoreGive(s_mutex);
                return ESP_FAIL;
            }
        }
    } else {
        memcpy(s_work_pixels, s_pixels, sizeof(s_work_pixels));
    }
    if (save_index(&s_work_index) != ESP_OK) {
        xSemaphoreGive(s_mutex);
        return ESP_FAIL;
    }

    char path[64];
    image_path(image_id, path, sizeof(path));
    unlink(path);
    s_index = s_work_index;
    memcpy(s_pixels, s_work_pixels, sizeof(s_pixels));
    xSemaphoreGive(s_mutex);
    return ESP_OK;
}

size_t picopal_image_count(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    size_t count = s_index.count;
    xSemaphoreGive(s_mutex);
    return count;
}

size_t picopal_image_selected_position(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    int position = selected_index(&s_index);
    xSemaphoreGive(s_mutex);
    return position < 0 ? 0 : (size_t)position + 1;
}

uint32_t picopal_image_selected_id(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    uint32_t id = s_index.selected_id;
    xSemaphoreGive(s_mutex);
    return id;
}

size_t picopal_image_ids(uint32_t *ids, size_t capacity)
{
    if (ids == NULL && capacity != 0) {
        return 0;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    size_t copied = s_index.count < capacity ? s_index.count : capacity;
    memcpy(ids, s_index.ids, copied * sizeof(ids[0]));
    xSemaphoreGive(s_mutex);
    return copied;
}

bool picopal_image_copy_selected(uint8_t *pixels, size_t length)
{
    if (pixels == NULL || length != PICOPAL_IMAGE_SIZE) {
        return false;
    }
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    bool available = s_index.count > 0;
    if (available) {
        memcpy(pixels, s_pixels, sizeof(s_pixels));
    }
    xSemaphoreGive(s_mutex);
    return available;
}

bool picopal_image_available(void)
{
    return picopal_image_count() > 0;
}

void picopal_image_render(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    picopal_framebuffer_copy(s_pixels, sizeof(s_pixels));
    xSemaphoreGive(s_mutex);
}
