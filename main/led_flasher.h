#pragma once
#include "sdkconfig.h"
#include "esp_err.h"
#include <stdbool.h>

esp_err_t led_flasher_start(void);
esp_err_t led_flasher_register_event();
