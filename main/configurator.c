#include "sdkconfig.h"
#include "configurator.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_console.h"
#include "nvs_keys.h"

nvs_handle_t nvs;

bool startup_wait_for_console_command(uint8_t seconds)
{
    static const char *tag = __func__;
    vTaskDelay(pdMS_TO_TICKS(3000));
    while (seconds > 0)
    {
        ESP_LOGI(tag, "=== Hit ENTER to configure. %d seconds left ===", seconds);
        for (int i = 0; i < 10; i++)
        {
            int ch = fgetc(stdin);
            if (ch == '\r' || ch == '\n')
            {
                return true;
            }
            vTaskDelay(pdMS_TO_TICKS(100));
        }
        seconds--;
    }
    ESP_LOGI(tag, "Timeout - entering normal operation mode");
    return false;
}

esp_err_t configurator_init(uint8_t wait_seconds)
{
    static const char *tag = __func__;
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(tag, "NVS partition corrupted or full. Erasing and retrying...");
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), tag, "Failed erasing NVS flash");
        ret = nvs_flash_init();
    }

    ESP_RETURN_ON_ERROR(ret, tag, "failed open nvs flash");
    ESP_RETURN_ON_ERROR(nvs_open(CONFIG_MITHBRIDGE2_STORAGE_NVS_NS, NVS_READONLY, &nvs), tag, "Unable to open nvs namespace '%s'.", CONFIG_MITHBRIDGE2_STORAGE_NVS_NS);

    if (startup_wait_for_console_command(wait_seconds))
    {
        ESP_LOGI(tag, "ENTERING CONFIGURATION MODE");
        return ESP_ERR_NOT_FINISHED;
    }
    ESP_LOGI(tag, "init complete");
    return ESP_OK;
}

const char *configurator_get_str_alloc(const char *key)
{
    static const char *tag = __func__;
    size_t len = 0;
    ESP_RETURN_ON_FALSE(nvs_get_str(nvs, key, NULL, &len) == ESP_OK, NULL, tag, "unable to get size for '%s' key", key);
    char *buf = calloc(len, sizeof(char));
    ESP_RETURN_ON_FALSE(buf != NULL, NULL, tag, "memory allocation failed for key '%s'", key);
    if (nvs_get_str(nvs, key, buf, &len) != ESP_OK)
    {
        free(buf);
        return NULL;
    }
    return buf;
}

esp_err_t configurator_get_str_to_buf(const char *key, char *buf, size_t len)
{
    static const char *tag = __func__;
    ESP_RETURN_ON_ERROR(nvs_get_str(nvs, key, buf, &len), tag, "unable to read '%s' key", key);
    return ESP_OK;
}

void configurator_set_str(const char *key, const char *value)
{
    nvs_handle_t nvs_w;
    ESP_ERROR_CHECK(nvs_open(CONFIG_MITHBRIDGE2_STORAGE_NVS_NS, NVS_READWRITE, &nvs_w));
    ESP_ERROR_CHECK(nvs_set_str(nvs_w, key, value));
    ESP_ERROR_CHECK(nvs_commit(nvs_w));
    nvs_close(nvs_w);
}
