#pragma once
#include <stddef.h>
#include "esp_err.h"

esp_err_t configurator_init(uint8_t waut_seconds);
esp_err_t configurator_get_str_to_buf(const char *key, char *buf, size_t len);
const char *configurator_get_str_alloc(const char *key);
void configurator_set_str(const char *key, const char *value);