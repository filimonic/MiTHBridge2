#include "sdkconfig.h"
#include "sensor_sender_influx_line_https.h"
#include <stdlib.h>
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_check.h"
#include "esp_http_client.h"
#include "esp_crt_bundle.h"

static char buffer[1024];
static char chunk_header[32];
static sensor_sender_influx_line_https_config_t cfg;

#define SENSOR_SENDER_INFLUX_LINE_HTTPS_MAC_BYTES_LEN_MAX 8

sensor_sender_influx_line_https_config_t *sensor_sender_influx_line_https_init(const char *url, const char *auth)
{
    static const char *tag = __func__;
    ESP_RETURN_ON_FALSE(url != NULL, NULL, tag, "url must not be NULL");

    cfg.url = strdup(url);
    if (auth != NULL)
    {
        cfg.auth = strdup(auth);
    }
    else
    {
        cfg.auth = NULL;
    }

    esp_mac_type_t mac_type = ESP_MAC_EFUSE_FACTORY;
    size_t mac_len = esp_mac_addr_len_get(mac_type);
    uint8_t *device_mac = malloc(mac_len);
    ESP_ERROR_CHECK(esp_read_mac(device_mac, mac_type));
    assert(mac_len <= SENSOR_SENDER_INFLUX_LINE_HTTPS_MAC_BYTES_LEN_MAX && mac_len > 0);
    cfg.sender_id = (char *)calloc(((mac_len * 3) - 1) + 1, sizeof(char));
    char *p = cfg.sender_id;
    for (size_t i = 0; i < mac_len; i++)
    {
        p += sprintf(p, "%02X:", device_mac[i]);
    }
    // Replace last colon witn \0
    p--;
    p[0] = '\0';
    free(device_mac);

    esp_http_client_config_t http_cfg = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .keep_alive_enable = true,

        // In this mode, TLS verifies if centificate has right SAN or CN, but ignores certificate chain validation
        .common_name = NULL,
        .skip_cert_common_name_check = false,
        .use_global_ca_store = false,
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    cfg.http_client = esp_http_client_init(&http_cfg);
    ESP_ERROR_CHECK(cfg.http_client == NULL ? ESP_FAIL : ESP_OK);

    if (cfg.auth != NULL)
    {
        ESP_ERROR_CHECK(esp_http_client_set_header(cfg.http_client, "Authorization", cfg.auth));
    }
    ESP_ERROR_CHECK(esp_http_client_set_header(cfg.http_client, "Transfer-Encoding", "chunked"));
    ESP_ERROR_CHECK(esp_http_client_set_header(cfg.http_client, "Content-Type", "text/plain; charset=utf-8"));
    return &cfg;
}

int sensor_sender_influx_line_https_send(sensor_data_t *sensor_data_list, int count, sensor_sender_influx_line_https_config_t *cfg)
{
    static const char *tag = __func__;
    int sensors_data_send_success_count = 0;

    esp_err_t err = esp_http_client_open(cfg->http_client, -1);
    if (err != ESP_OK)
    {
        ESP_LOGE(tag, "Failed to open connection: %s", esp_err_to_name(err));
        return 0;
    }

    for (int i = 0; i < count; i++)
    {
        char *p = buffer;
        sensor_data_t *sd = &sensor_data_list[i];
        int id_len = sd->id_kind == SENSOR_DATA_ID_KIND_MAC6 ? 6 : 8;
        p += sprintf(p, "sensor,sender_id=%s,id=", cfg->sender_id);
        for (int id_idx = 0; id_idx < id_len; id_idx++)
        {
            p += sprintf(p, "%02X:", sd->id[id_idx]);
        }
        p--; // Drop last :
        p += sprintf(p, " ");

        // Temperature
        if (sd->field_presence & SENSOR_DATA_HAS_TEMPERATURE)
        {
            p += sprintf(p, "temperature=%.2f,", (float)sd->temperature / 100.0f);
        }

        // Humidity
        if (sd->field_presence & SENSOR_DATA_HAS_HUMIDITY)
        {
            p += sprintf(p, "humidity=%.2f,", (float)sd->humidity / 100.0f);
        }

        // Voltage
        if (sd->field_presence & SENSOR_DATA_HAS_VOLTAGE)
        {
            p += sprintf(p, "voltage=%.3f,", (float)sd->voltage / 1000.0f);
        }

        // Battery Level
        if (sd->field_presence & SENSOR_DATA_HAS_BATTERY_LEVEL)
        {
            p += sprintf(p, "battery_level=%ui,", sd->battery_level);
        }

        // RSSI
        if (sd->field_presence & SENSOR_DATA_HAS_RSSI)
        {
            p += sprintf(p, "rssi=%di,", sd->rssi);
        }

        // TX Power
        if (sd->field_presence & SENSOR_DATA_HAS_TX_POWER)
        {
            p += sprintf(p, "tx_power=%di,", sd->tx_power);
        }

        // Uptime
        if (sd->field_presence & SENSOR_DATA_HAS_UPTIME)
        {
            p += sprintf(p, "uptime=%lui,", sd->uptime);
        }

        // BLE Primary phy
        if (sd->field_presence & SENSOR_DATA_HAS_BLE_PRIMARY_PHY)
        {
            int8_t val = -127;
            switch (sd->ble_primary_phy_kind)
            {
            case SENSOR_DATA_BLE_PRIMARY_PHY_KIND_1M:
                val = 1;
                break;
            case SENSOR_DATA_BLE_PRIMARY_PHY_KIND_CODED:
                val = 3;
                break;
            }
            p += sprintf(p, "ble_primary_phy=%di,", val);
        }

        // BLE Secondary phy
        if (sd->field_presence & SENSOR_DATA_HAS_BLE_SECONDARY_PHY)
        {
            int8_t val = -127;
            switch (sd->ble_secondary_phy_kind)
            {
            case SENSOR_DATA_BLE_SECONDARY_PHY_KIND_1M:
                val = 1;
                break;
            case SENSOR_DATA_BLE_SECONDARY_PHY_KIND_2M:
                val = 2;
                break;
            case SENSOR_DATA_BLE_SECONDARY_PHY_KIND_CODED:
                val = 3;
                break;
            }
            p += sprintf(p, "ble_secondary_phy=%di,", val);
        }

        // Fix last comma
        if (*(p - 1) != ',')
        {
            sensors_data_send_success_count++;
            continue;
        }
        p--;
        p += sprintf(p, "\n");

        ESP_LOGW(tag, "Sending chunk '%.*s'", strlen(buffer) - 1, buffer);

        size_t line_len = p - buffer;
        int chunk_header_len = sprintf(chunk_header, "%X\r\n", line_len);

        if ((esp_http_client_write(cfg->http_client, chunk_header, chunk_header_len) < 0) || (esp_http_client_write(cfg->http_client, buffer, line_len) < 0) || (esp_http_client_write(cfg->http_client, "\r\n", 2) < 0))
        {
            ESP_LOGE(tag, "Failed to write HTTP chunk at index %d", i);
            break;
        }
        sensors_data_send_success_count++;
    }

    esp_http_client_write(cfg->http_client, "0\r\n\r\n", 5);

    int post_process_len = esp_http_client_fetch_headers(cfg->http_client);
    if (post_process_len >= 0)
    {
        int status = esp_http_client_get_status_code(cfg->http_client);
        if (status == 204 || status == 200)
        {
            ESP_LOGI(tag, "write successful. Status: %d", status);
        }
        else
        {
            ESP_LOGW(tag, "returned error status: %d", status);
        }
    }
    else
    {
        ESP_LOGE(tag, "Error parsing server response headers");
    }

    esp_http_client_close(cfg->http_client);
    return sensors_data_send_success_count;
};

#undef SENSOR_SENDER_INFLUX_LINE_HTTPS_MAC_BYTES_LEN_MAX