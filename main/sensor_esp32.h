#pragma once
#include "esp_err.h"

esp_err_t sensor_esp32_init();
esp_err_t sensor_esp32_fill(void *sensor_data_ptr);
