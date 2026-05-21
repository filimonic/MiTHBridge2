#include "sdkconfig.h"
#include "sensor_ble.h"
#include "stdint.h"
#include "stdbool.h"
#include "host/ble_gap.h"
#include "host/ble_hs_adv.h"
#include "sensor.h"
#include "esp_check.h"

typedef struct __attribute__((packed)) _pvvx_adv_field_data_t  {
    uint16_t uuid;
    uint8_t rev_mac[6]; // Reversed mac
    int16_t temperature; // x 0.01 °C
    uint16_t humidity; // x 0.01 %
    uint16_t battery_mv; // x 0.001 V
    uint8_t battery_level; // 0..100 %
    uint8_t counter;
    uint8_t flags;
} pvvx_adv_field_data_t;

typedef struct __attribute__((packed)) _atc1441_adv_field_data_t  {
    uint16_t uuid;
    uint8_t mac[6]; // mac
    int16_t temperature; // x 0.1 °C
    uint8_t humidity; // x 1%
    uint8_t battery_level; // 0..100 %
    uint16_t battery_mv; // x 0.001 V
    uint8_t frame_counter;
} atc1441_adv_field_data_t;

typedef enum __attribute__((packed)) {
    ADV_DATA_FORMAT_UNKNOWN = 0,
    ADV_DATA_FORMAT_PVVX,
    ADV_DATA_FORMAT_ATC1441,
} field_data_format_t;

field_data_format_t detect_uuid16_field_data_dormat(const struct ble_hs_adv_field * field);
void sensor_data_copy_pvvx(sensor_data_t*, pvvx_adv_field_data_t*);
void sensor_data_copy_atc1441(sensor_data_t*, atc1441_adv_field_data_t*);


int sensor_ble_hs_adv_parse_cb(const struct ble_hs_adv_field * field, void * sensor_data_ptr) 
{
    static const char *tag = __func__;
    // This function should return 0 to stop parsing adv packet fields or non-0 to continue.
    if (field->length == 0) 
    {
        return 0;
    }

    sensor_data_t *sensor_data = (sensor_data_t *)sensor_data_ptr;

    switch (field->type)
    {
        case BLE_HS_ADV_TYPE_INCOMP_NAME:
        case BLE_HS_ADV_TYPE_COMP_NAME:
            return 1;
            break;
        case BLE_HS_ADV_TYPE_SVC_DATA_UUID16:
            field_data_format_t df = detect_uuid16_field_data_dormat(field);
            switch (df)
            {
                case ADV_DATA_FORMAT_PVVX:
                    sensor_data_copy_pvvx(sensor_data, (pvvx_adv_field_data_t*)field->value);
                    return 1;
                case ADV_DATA_FORMAT_ATC1441:
                    sensor_data_copy_atc1441(sensor_data, (atc1441_adv_field_data_t*)field->value);
                    return 1;
                case ADV_DATA_FORMAT_UNKNOWN:
                default:
                    return 0;
            }
            ESP_RETURN_ON_FALSE(false, 1, tag, "reached unreachable point");
        default:
            return 0;
    }
    return 1;
}

field_data_format_t detect_uuid16_field_data_dormat(const struct ble_hs_adv_field * field) 
{
    uint8_t value_length = field->length - 1;
    if (value_length == sizeof(pvvx_adv_field_data_t) && ((pvvx_adv_field_data_t*)field->value)->uuid == 0x181A) 
    {
        return ADV_DATA_FORMAT_PVVX;
    }
    if (value_length == sizeof(atc1441_adv_field_data_t) && ((atc1441_adv_field_data_t*)field->value)->uuid == 0x181A) 
    {
        return ADV_DATA_FORMAT_ATC1441;
    }
    return ADV_DATA_FORMAT_UNKNOWN;
}

void sensor_data_copy_pvvx(sensor_data_t* sensor_data, pvvx_adv_field_data_t * pvvx_data)
{
    sensor_data->battery_level = pvvx_data->battery_level; // [x1%, x1%]
    sensor_data->humidity = pvvx_data->humidity; // [x0.01%, x0.01%]
    sensor_data->temperature = pvvx_data->temperature; // [x0.01°C, x0.01°C]
    sensor_data->voltage = pvvx_data->battery_mv; // [x0.001V, x0.001V]
    sensor_data->field_presence |= SENSOR_DATA_HAS_BATTERY_LEVEL | SENSOR_DATA_HAS_VOLTAGE | SENSOR_DATA_HAS_HUMIDITY | SENSOR_DATA_HAS_TEMPERATURE;

    // MAC is reversed in PVVX format
    sensor_data->id_kind = SENSOR_DATA_ID_KIND_MAC6;
    sensor_data->id[0] = pvvx_data->rev_mac[5];
    sensor_data->id[1] = pvvx_data->rev_mac[4];
    sensor_data->id[2] = pvvx_data->rev_mac[3];
    sensor_data->id[3] = pvvx_data->rev_mac[2];
    sensor_data->id[4] = pvvx_data->rev_mac[1];
    sensor_data->id[5] = pvvx_data->rev_mac[0];
}

void sensor_data_copy_atc1441(sensor_data_t* sensor_data, atc1441_adv_field_data_t * atc1441_data)
{
    sensor_data->battery_level = atc1441_data->battery_level; // [x1%, x1%]
    sensor_data->voltage = atc1441_data->battery_mv; // [x0.001V, x0.001V]

    // ⚠️ Humidity: ATC1441 has humidity in 1%, need conversion to 0.01% units
    sensor_data->humidity = atc1441_data->humidity * 100; // [x0.01%, x1%]
    // ⚠️ Temperature: ATC1441 has temperature in 0.1°C, need conversion to 0.01°C units
    sensor_data->temperature = atc1441_data->temperature * 10; // [x0.01°C, x0.1°C]

    sensor_data->field_presence |= SENSOR_DATA_HAS_BATTERY_LEVEL | SENSOR_DATA_HAS_VOLTAGE | SENSOR_DATA_HAS_HUMIDITY | SENSOR_DATA_HAS_TEMPERATURE;

    // MAC is forvard in ATC1441 format
    sensor_data->id_kind = SENSOR_DATA_ID_KIND_MAC6;
    sensor_data->id[0] = atc1441_data->mac[0];
    sensor_data->id[1] = atc1441_data->mac[1];
    sensor_data->id[2] = atc1441_data->mac[2];
    sensor_data->id[3] = atc1441_data->mac[3];
    sensor_data->id[4] = atc1441_data->mac[4];
    sensor_data->id[5] = atc1441_data->mac[5];
}