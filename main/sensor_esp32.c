#include "sdkconfig.h"
#include "sensor_esp32.h"
#include "sensor.h"
#include "esp_err.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_check.h"

#include "driver/temperature_sensor.h"
#include <math.h>

uint8_t *device_id;
uint8_t device_id_kind;
size_t device_id_size;
temperature_sensor_handle_t temperature_sensor;

esp_err_t sensor_esp32_init()
{
    static const char *tag = __func__;
    esp_mac_type_t mac_type = ESP_MAC_EFUSE_FACTORY;
    device_id_size = esp_mac_addr_len_get(mac_type);
    switch (device_id_size)
    {
    case 6:
        device_id_kind = SENSOR_DATA_ID_KIND_MAC6;
        device_id_size = 6;
        break;
    case 8:
        device_id_kind = SENSOR_DATA_ID_KIND_MAC8;
        device_id_size = 8;
        break;
    default:
        ESP_LOGE(tag, "MAC length %zu not expected", device_id_size);
        return ESP_FAIL;
    }
    device_id = malloc(device_id_size);
    ESP_ERROR_CHECK(esp_read_mac(device_id, mac_type));

    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temperature_sensor));
    ESP_ERROR_CHECK(temperature_sensor_enable(temperature_sensor));

    return ESP_OK;
}

esp_err_t sensor_esp32_fill(void *sensor_data_ptr)
{
    static const char *tag = __func__;
    sensor_data_t *sd = (sensor_data_t *)sensor_data_ptr;
    sd->id_kind = device_id_kind;
    memcpy(sd->id, device_id, device_id_size);

    float temperature;
    ESP_ERROR_CHECK(temperature_sensor_get_celsius(temperature_sensor, &temperature));
    sd->temperature = (int32_t)lroundf(temperature * 100.0f);
    int64_t uptime = esp_timer_get_time();
    ESP_RETURN_ON_FALSE(uptime > 0, ESP_FAIL, tag, "uptime overflow");
    sd->uptime = (uint32_t)(uptime / 1000000LL);
    sd->field_presence = SENSOR_DATA_HAS_TEMPERATURE | SENSOR_DATA_HAS_UPTIME;
    return ESP_OK;
}
