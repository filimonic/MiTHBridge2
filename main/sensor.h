#pragma once
#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

typedef enum : uint8_t
{
    SENSOR_DATA_ID_KIND_MAC8 = 0,
    SENSOR_DATA_ID_KIND_MAC6 = 1,
} sensor_data_id_kind_t;

typedef enum : uint8_t
{
    SENSOR_DATA_BLE_PRIMARY_PHY_KIND_1M = 0,
    SENSOR_DATA_BLE_PRIMARY_PHY_KIND_CODED = 1,
} sensor_data_ble_primary_phy_kind_t;

typedef enum : uint8_t
{
    SENSOR_DATA_BLE_SECONDARY_PHY_KIND_NONE = 0,
    SENSOR_DATA_BLE_SECONDARY_PHY_KIND_1M = 1,
    SENSOR_DATA_BLE_SECONDARY_PHY_KIND_2M = 2,
    SENSOR_DATA_BLE_SECONDARY_PHY_KIND_CODED = 3,
} sensor_data_ble_secondary_phy_kind_t;

typedef enum
{
    SENSOR_DATA_HAS_BLE_PRIMARY_PHY = (1 << 0),
    SENSOR_DATA_HAS_BLE_SECONDARY_PHY = (1 << 1),
    SENSOR_DATA_HAS_TEMPERATURE = (1 << 2),
    SENSOR_DATA_HAS_HUMIDITY = (1 << 3),
    SENSOR_DATA_HAS_VOLTAGE = (1 << 4),
    SENSOR_DATA_HAS_BATTERY_LEVEL = (1 << 5),
    SENSOR_DATA_HAS_RSSI = (1 << 6),
    SENSOR_DATA_HAS_TX_POWER = (1 << 7),
    SENSOR_DATA_HAS_UPTIME = (1 << 8),
    // Check sensor_data_t.field_presence type if adding more !
} sensor_data_field_presence_t;

typedef struct __attribute__((packed)) _sensor_data_t
{
    uint16_t field_presence;
    // sensor_data_ble_primary_phy_kind_t
    // See `SENSOR_DATA_HAS_BLE_PRIMARY_PHY` bit in field_presence to determine if this field value is correct.
    // 0: SENSOR_DATA_BLE_PRIMARY_PHY_KIND_1M
    // 1: SENSOR_DATA_BLE_PRIMARY_PHY_KIND_CODED
    uint8_t ble_primary_phy_kind : 1;

    // sensor_data_ble_secondary_phy_kind_t
    // See `SENSOR_DATA_HAS_BLE_SECONDARY_PHY` bit in field_presence to determine if this field value is correct.
    // If `has_ble_secondary_phy == true`, then:
    // 0: SENSOR_DATA_BLE_SECONDARY_PHY_KIND_NONE
    // 1: SENSOR_DATA_BLE_SECONDARY_PHY_KIND_1M
    // 2: SENSOR_DATA_BLE_SECONDARY_PHY_KIND_2M
    // 3: SENSOR_DATA_BLE_SECONDARY_PHY_KIND_CODED
    uint8_t ble_secondary_phy_kind : 2;

    // sensor_data_id_kind_t :
    // - SENSOR_DATA_ID_KIND_MAC8 for 8-byte MAC id
    // - SENSOR_DATA_ID_KIND_MAC6 for 6-byte MAC id
    uint8_t id_kind : 1;
    // See `id_kind` (sensor_data_id_kind_t) to get length of this MAC
    uint8_t id[8];
    // x 0.01 °C
    // Temperature in centi-celsius degrees ( 00.1 °C ), value -1234 means -12.34°C.
    // x 0.01 %
    int32_t temperature;
    // Relative humidity in centi-percert ( 0.01 % ), value 8765 means 87.65%.
    uint16_t humidity;
    // x 0.001 V
    // Voltage in 1/1000 V (milivolt), value 3212 means 3.212 V.
    uint32_t voltage;
    // x 1 %
    // Battery level in percent: value 92 means 92%.
    uint8_t battery_level;
    // x 1 dbm
    // RSSI.
    int8_t rssi;
    // x 1 dbm
    // TX Power.
    int8_t tx_power;
    // x 1 second
    // Uptime (seconds)
    uint32_t uptime;

} sensor_data_t;

// return 0 if ok, other on fail
typedef int (*sensor_data_collection_process_fn_t)(sensor_data_t *, int count, void *arg);

typedef struct _sensor_data_collection_t
{
    sensor_data_t *buf;
    int capacity;
    int count;
    sensor_data_collection_process_fn_t process_fn;
    SemaphoreHandle_t mutex;
} sensor_data_collection_t;

sensor_data_collection_t sensor_data_collection_init(int, sensor_data_collection_process_fn_t);
bool sensor_data_collection_add(sensor_data_collection_t *, const sensor_data_t *);
bool sensor_data_is_complete(const sensor_data_t *);
void sensor_data_collection_clear(sensor_data_collection_t *);
int sensor_data_collection_process(sensor_data_collection_t *, void *arg);
