#pragma once
#include "stdint.h"
#include "stdbool.h"
#include "host/ble_gap.h"

int sensor_ble_hs_adv_parse_cb(const struct ble_hs_adv_field *field, void *sensor_data_ptr);
