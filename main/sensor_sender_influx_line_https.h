#pragma once
#include "stdint.h"
#include "sensor.h"
#include "esp_http_client.h"

typedef struct _sensor_sender_influx_line_https_config_t
{
    char *url;
    char *auth;
    char *sender_id;
    esp_http_client_handle_t http_client;
} sensor_sender_influx_line_https_config_t;

sensor_sender_influx_line_https_config_t *sensor_sender_influx_line_https_init(const char *url, const char *auth);
int sensor_sender_influx_line_https_send(sensor_data_t *, int count, sensor_sender_influx_line_https_config_t *arg);
