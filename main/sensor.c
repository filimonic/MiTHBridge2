#include "sdkconfig.h"
#include "sensor.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "string.h"

sensor_data_collection_t sensor_data_collection_init(int capacity, sensor_data_collection_process_fn_t process_fn)
{
    sensor_data_collection_t col = {
        .buf = malloc(capacity * sizeof(sensor_data_t)),
        .capacity = capacity,
        .count = 0,
        .process_fn = process_fn,
    };
    col.mutex = xSemaphoreCreateMutex();
    assert(col.buf != NULL);
    assert(col.mutex != NULL);
    return col;
}

bool sensor_data_collection_add(sensor_data_collection_t *collection, const sensor_data_t *sensor_data)
{
    if (xSemaphoreTake(collection->mutex, portMAX_DELAY) == pdTRUE)
    {
        static const char *tag = __func__;
        if (collection->count >= (collection->capacity))
        {
            ESP_LOGD(tag, "collection is full (count=%d, capacity=%d)", collection->count, collection->capacity);
            xSemaphoreGive(collection->mutex);
            return false;
        }
        sensor_data_t *new_element = &collection->buf[collection->count];
        sensor_data_t *existing_element = NULL;
        for (int i = 0; i < collection->count; i++)
        {
            if (memcmp(collection->buf[i].id, sensor_data->id, sizeof(sensor_data->id)) == 0)
            {
                ESP_LOGD(tag, "sensor data replaces existing (id=%d, count=%d, capacity=%d, new_mac=\"" MACSTR "\", old_mac=\"" MACSTR "\")", i, collection->count, collection->capacity, MAC2STR(collection->buf[i].id), MAC2STR(sensor_data->id));
                existing_element = &collection->buf[i];
                break;
            }
        }

        if (existing_element != NULL)
        {
            ESP_LOGD(tag, "using existing element in collection");
            memcpy(existing_element, sensor_data, sizeof(sensor_data_t));
        }
        else
        {
            ESP_LOGD(tag, "adding new element to collection");
            memcpy(new_element, sensor_data, sizeof(sensor_data_t));
            collection->count++;
        }
        xSemaphoreGive(collection->mutex);
    }
    return true;
}

void sensor_data_collection_clear(sensor_data_collection_t *collection)
{
    if (xSemaphoreTake(collection->mutex, portMAX_DELAY) == pdTRUE)
    {
        collection->count = 0;
        xSemaphoreGive(collection->mutex);
    }
}

int sensor_data_collection_process(sensor_data_collection_t *collection, void *arg)
{
    int result = -1;
    if (xSemaphoreTake(collection->mutex, portMAX_DELAY) == pdTRUE)
    {
        result = collection->process_fn(collection->buf, collection->count, arg);
        xSemaphoreGive(collection->mutex);
    }
    return result;
}

bool sensor_data_is_complete(const sensor_data_t *sd)
{
    return 0 != ((sd->field_presence & (SENSOR_DATA_HAS_BATTERY_LEVEL | SENSOR_DATA_HAS_HUMIDITY | SENSOR_DATA_HAS_RSSI | SENSOR_DATA_HAS_TEMPERATURE | SENSOR_DATA_HAS_VOLTAGE)) && *(uint64_t *)(sd->id) != 0);
}