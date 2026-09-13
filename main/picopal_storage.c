#include "picopal_storage.h"

#include <errno.h>
#include <sys/stat.h>

#include "esp_check.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define PICOPAL_STORAGE_PARTITION "storage"

static const char *TAG = "storage";

static esp_err_t init_nvs(void)
{
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES ||
        result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS requires reinitialization");
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "NVS erase failed");
        result = nvs_flash_init();
    }
    return result;
}

static esp_err_t create_directory(const char *path)
{
    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return ESP_OK;
    }
    ESP_LOGE(TAG, "Could not create %s (errno=%d)", path, errno);
    return ESP_FAIL;
}

esp_err_t picopal_storage_info(size_t *total_bytes, size_t *used_bytes)
{
    if (total_bytes == NULL || used_bytes == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return esp_littlefs_info(
        PICOPAL_STORAGE_PARTITION,
        total_bytes,
        used_bytes
    );
}

esp_err_t picopal_storage_init(void)
{
    ESP_RETURN_ON_ERROR(init_nvs(), TAG, "NVS initialization failed");

    const esp_vfs_littlefs_conf_t config = {
        .base_path = PICOPAL_STORAGE_BASE_PATH,
        .partition_label = PICOPAL_STORAGE_PARTITION,
        .format_if_mount_failed = true,
        .dont_mount = false,
    };
    ESP_RETURN_ON_ERROR(
        esp_vfs_littlefs_register(&config),
        TAG,
        "LittleFS mount failed"
    );
    ESP_RETURN_ON_ERROR(
        create_directory(PICOPAL_IMAGES_PATH),
        TAG,
        "Images directory failed"
    );
    ESP_RETURN_ON_ERROR(
        create_directory(PICOPAL_WEB_PATH),
        TAG,
        "Web directory failed"
    );

    size_t total_bytes = 0;
    size_t used_bytes = 0;
    ESP_RETURN_ON_ERROR(
        picopal_storage_info(&total_bytes, &used_bytes),
        TAG,
        "Storage information failed"
    );
    ESP_LOGI(
        TAG,
        "LittleFS ready: %u / %u bytes used",
        (unsigned)used_bytes,
        (unsigned)total_bytes
    );
    return ESP_OK;
}
