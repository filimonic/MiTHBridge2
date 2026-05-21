#include "sdkconfig.h"
#include <stdio.h>
#include "checks.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_netif.h"
#include "esp_err.h"
#include "esp_check.h"
#include "esp_event_base.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs_keys.h"
#include "driver/gpio.h"

#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "esp_console.h"

#include "sensor_ble.h"
#include "sensor_esp32.h"
#include "sensor.h"
#include "sensor_sender_influx_line_https.h"

#include "configurator.h"
#include "configurator_console.h"
#include "xiao_esp32c6_amp.h"
#include "led_flasher.h"

#define MINUTES_TO_MICROSECONDS_S64(m) ((int64_t)(m) * 60000000L)
#define SECONDS_TO_MICROSECONDS_U64(s) ((uint64_t)(s) * 1000000L)

esp_err_t setup_gpio();
static esp_err_t nimble_init_wrapped(void);
static esp_err_t wifi_creds_setup();
static void wifi_cb(void *arg, esp_event_base_t base, int32_t id, void *data);
static void ble_cb_stack_sync(void);
static int ble_cb_disc(struct ble_gap_event *event, void *arg);
static void send_task(void *arg);
static void sensor_esp32_task(void *arg);

sensor_data_collection_t sensor_data_collection;
sensor_sender_influx_line_https_config_t *sensor_sender_influx_line_https_cfg;

void app_main(void)
{
    static const char *tag = __func__;
    esp_err_t err = configurator_init(15);
    if (err == ESP_ERR_NOT_FINISHED)
    {
        configurator_console_run();
        return;
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(setup_gpio());
    ESP_ERROR_CHECK(led_flasher_start());

    // Init senders
    sensor_sender_influx_line_https_cfg = sensor_sender_influx_line_https_init(
        configurator_get_str_alloc(MITHBRIDGE2_NVS_INFLUX_LINE_HTTPS_URL_KEY),
        configurator_get_str_alloc(MITHBRIDGE2_NVS_INFLUX_LINE_HTTPS_AUTH_KEY));

    if (sensor_sender_influx_line_https_cfg == NULL)
    {
        // run config;
    }

    // Init in-memory sensor data collection
    sensor_data_collection = sensor_data_collection_init(128, (void *)&sensor_sender_influx_line_https_send);

    // Wi-Fi
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_cb, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_cb, NULL, NULL);
    ESP_ERROR_CHECK(wifi_creds_setup());
    ESP_ERROR_CHECK(esp_wifi_start());

    BaseType_t result = xTaskCreate(send_task, "send_task", 8192, NULL, tskIDLE_PRIORITY + 1, NULL);
    if (result != pdPASS)
    {
        ESP_LOGE(tag, "failed creating task: %d", result);
        ESP_ERROR_CHECK(ESP_FAIL);
    }
    result = xTaskCreate(sensor_esp32_task, "sensor_esp32_task", 1024, (void *)&sensor_data_collection, tskIDLE_PRIORITY + 1, NULL);
    if (result != pdPASS)
    {
        ESP_LOGE(tag, "failed creating task: %d", result);
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    vTaskDelete(NULL);
}

esp_err_t setup_gpio()
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    io_conf.pin_bit_mask |= (1ULL << CONFIG_MITHBRIDGE2_LED_FLASHER_LED_GPIO);
    io_conf.pin_bit_mask |= (1ULL << XIAO_ESP32C6_EXT_ANTENNA_RFSWITCH_ENABLE_GPIO);
    io_conf.pin_bit_mask |= (1ULL << XIAO_ESP32C6_EXT_ANTENNA_RFSWITCH_EXT_ANT_GPIO);
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_set_level(CONFIG_MITHBRIDGE2_LED_FLASHER_LED_GPIO, (CONFIG_MITHBRIDGE2_LED_FLASHER_INVERTED ? 1 : 0))); // Turn off LED if inverted

    // Setup antenna
    vTaskDelay(pdMS_TO_TICKS(XIAO_ESP32C6_EXT_ANTENNA_RFSWITCH_ENABLE_DELAY));
    ESP_ERROR_CHECK(gpio_set_level(XIAO_ESP32C6_EXT_ANTENNA_RFSWITCH_ENABLE_GPIO, XIAO_ESP32C6_EXT_ANTENNA_RFSWITCH_ENABLE_LEVEL)); // Enable RF Switch
    vTaskDelay(pdMS_TO_TICKS(XIAO_ESP32C6_EXT_ANTENNA_RFSWITCH_ENABLE_DELAY));
    ESP_ERROR_CHECK(gpio_set_level(XIAO_ESP32C6_EXT_ANTENNA_RFSWITCH_EXT_ANT_GPIO, XIAO_ESP32C6_EXT_ANTENNA_RFSWITCH_EXT_ANT_LEVEL)); // Enable RF Switch
    vTaskDelay(pdMS_TO_TICKS(XIAO_ESP32C6_EXT_ANTENNA_RFSWITCH_ENABLE_DELAY));

    return ESP_OK;
}

static void ble_host_task(void *param)
{
    static const char *tag = __func__;
    ESP_LOGI(tag, "BLE Host Task Started");
    nimble_port_run();
    ESP_LOGE(tag, "BLE host exited");
    ESP_ERROR_CHECK(ESP_FAIL);
}

static esp_err_t nimble_init_wrapped(void)
{
    static const char *tag = __func__;
    ESP_RETURN_ON_ERROR(nimble_port_init(), tag, "failed init NimBLE port");
    ble_hs_cfg.sync_cb = ble_cb_stack_sync;
    nimble_port_freertos_init(ble_host_task);
    return ESP_OK;
}

static esp_err_t nimble_deinit_wrapped(void)
{
    nimble_port_freertos_deinit();
    nimble_port_deinit();
    return ESP_OK;
}

static esp_err_t wifi_creds_setup()
{
    static const char *tag = __func__;
    wifi_config_t wifi_cfg = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .pmf_cfg = {
                .capable = true,
                .required = false,
            }}};

    ESP_RETURN_ON_ERROR(configurator_get_str_to_buf(MITHBRIDGE2_NVS_WIFI_SSID_KEY, (char *)wifi_cfg.sta.ssid, sizeof(wifi_cfg.sta.ssid)), tag, "Unable to get %s config string", MITHBRIDGE2_NVS_WIFI_SSID_KEY);
    ESP_RETURN_ON_ERROR(configurator_get_str_to_buf(MITHBRIDGE2_NVS_WIFI_PSK_KEY, (char *)wifi_cfg.sta.password, sizeof(wifi_cfg.sta.ssid)), tag, "Unable to get %s config string", MITHBRIDGE2_NVS_WIFI_PSK_KEY);
    ESP_LOGI(tag, "Setup Wi-Fi AP SSID: '%s'", wifi_cfg.sta.ssid);
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg), tag, "failed to apply Wi-Fi STA config");
    return ESP_OK;
}

static int ble_cb_disc(struct ble_gap_event *event, void *arg)
{
    uint8_t *mac;
    static const char *tag = __func__;
    if (event->type == BLE_GAP_EVENT_EXT_DISC)
    {
        mac = event->ext_disc.addr.val;

        struct ble_gap_ext_disc_desc *adv = &event->ext_disc;
        mac = adv->addr.val;
        if (adv->data_status == BLE_GAP_EXT_ADV_DATA_STATUS_COMPLETE)
        {
            sensor_data_t sd;
            memset(&sd, 0, sizeof(sd));
            if (adv->rssi != 127)
            {
                sd.field_presence |= SENSOR_DATA_HAS_RSSI;
                sd.rssi = adv->rssi;
            }

            if (adv->tx_power != 127)
            {
                sd.field_presence |= SENSOR_DATA_HAS_TX_POWER;
                sd.tx_power = adv->tx_power;
            }

            switch (adv->prim_phy)
            {
            case BLE_HCI_LE_PHY_1M:
            case BLE_HCI_LE_PHY_CODED:
                sd.field_presence |= SENSOR_DATA_HAS_BLE_PRIMARY_PHY;
                sd.ble_primary_phy_kind = (adv->prim_phy == BLE_HCI_LE_PHY_1M)
                                              ? SENSOR_DATA_BLE_PRIMARY_PHY_KIND_1M
                                              : SENSOR_DATA_BLE_PRIMARY_PHY_KIND_CODED;
                break;
            }

            switch (adv->sec_phy)
            {
            case BLE_HCI_LE_PHY_1M:
            case BLE_HCI_LE_PHY_2M:
            case BLE_HCI_LE_PHY_CODED:
                sd.field_presence |= SENSOR_DATA_HAS_BLE_SECONDARY_PHY;
                sd.ble_secondary_phy_kind = (adv->sec_phy == BLE_HCI_LE_PHY_1M)
                                                ? SENSOR_DATA_BLE_SECONDARY_PHY_KIND_1M
                                            : (adv->sec_phy == BLE_HCI_LE_PHY_2M)
                                                ? SENSOR_DATA_BLE_SECONDARY_PHY_KIND_2M
                                                : SENSOR_DATA_BLE_SECONDARY_PHY_KIND_CODED;
                break;
            }

            int adv_parse_result = ble_hs_adv_parse(adv->data, adv->length_data, &sensor_ble_hs_adv_parse_cb, (void *)&sd);
            if (adv_parse_result == 0 && sensor_data_is_complete(&sd))
            {
                ESP_LOGI(tag, "Parsed sensor data from sensor " MACSTR, MAC2STR(mac));
                if (!sensor_data_collection_add((sensor_data_collection_t *)arg, &sd))
                {
                    ESP_LOGD(tag, "Unable to add sensor data");
                }
                else
                {
                    led_flasher_register_event();
                }
            }
        }
    }
    return 0;
}

static void ble_cb_stack_sync(void)
{
    static const char *tag = __func__;
    ESP_LOGI(tag, "Starting BLE Listener ...");
    struct ble_gap_ext_disc_params disc_params;
    memset(&disc_params, 0, sizeof(disc_params));
    disc_params.disable_observer_mode = 0;
    disc_params.passive = 1;
    disc_params.window = 0x0020;
    disc_params.itvl = 0x0080;

    int ret_int = ble_gap_ext_disc(
        BLE_HCI_ADV_OWN_ADDR_PUBLIC /* own_addr_type. 0 = BLE_ADDR_TYPE_PUBLIC */,
        0 /* duration. 0 = no expiration */,
        0 /* period. 0 = scan continuously */,
        0 /* filter_duplicates. 0 = no */,
        BLE_HCI_SCAN_FILT_NO_WL /* filter_policy */,
        0 /* limited. 0 = not limited*/,
        &disc_params /* uncoded_params */,
        &disc_params /* coded_params */,
        &ble_cb_disc /* cb */,
        &sensor_data_collection /* cb_arg */);
    if (ret_int != 0)
    {
        ESP_LOGE(tag, "ble_gap_disc() error: %d", ret_int);
        ESP_ERROR_CHECK(ESP_FAIL);
    }
}

static void wifi_reconnect_timer_cb(void *arg)
{
    static const char *tag = __func__;
    ESP_LOGI(tag, "Attempting to reconnect");
    ESP_ERROR_CHECK(esp_wifi_connect());
}

static void wifi_cb(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    static const char *tag = __func__;
    static esp_timer_handle_t reconnect_timer = NULL;
    static bool ble_initialized = false;
    if (base == WIFI_EVENT)
    {
        switch (id)
        {
        case WIFI_EVENT_STA_START:
            ESP_LOGI(tag, "Wi-Fi STA started, connecting ...");
            esp_wifi_connect();
            return;
        case WIFI_EVENT_STA_DISCONNECTED:
            if (ble_initialized)
            {
                ESP_ERROR_CHECK(nimble_deinit_wrapped());
                ble_initialized = false;
            }
            ESP_LOGW(tag, "Wi-Fi STA disconnected.");
            if (reconnect_timer == NULL)
            {
                esp_timer_create_args_t timer_cfg = {
                    .callback = &wifi_reconnect_timer_cb,
                    .name = "w_reconnect",
                };
                ESP_ERROR_CHECK(esp_timer_create(&timer_cfg, &reconnect_timer));
            }
            ESP_ERROR_CHECK(esp_timer_start_once(reconnect_timer, SECONDS_TO_MICROSECONDS_U64(CONFIG_MITHBRIDGE2_WIFI_RECONNECT_DELAY_SECONDS)));
            return;
        }
    }
    else if (base == IP_EVENT)
    {
        switch (id)
        {
        case IP_EVENT_STA_GOT_IP:
            if (!ble_initialized)
            {
                ESP_ERROR_CHECK(nimble_init_wrapped());
                ble_initialized = true;
            }
            return;
        }
    }
    ESP_LOGI(tag, "Unhandled event: base=%s; id=%d", base, id);
}

static void send_task(void *arg)
{
    static const char *tag = __func__;
    int64_t last_sent_timestamp = 0;
    while (true)
    {
        for (int i = CONFIG_MITHBRIDGE2_SEND_INTERVAL_SECONDS; i > 0; i--)
        {
            ESP_LOGI(tag, "delay before sending data (%d seconds left)", i);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        ESP_LOGI(tag, "Started sending data for %d sensors", sensor_data_collection.count);
        if (sensor_data_collection_process(&sensor_data_collection, sensor_sender_influx_line_https_cfg) > 0)
        {
            sensor_data_collection_clear(&sensor_data_collection);
            last_sent_timestamp = esp_timer_get_time();
        }

        if ((esp_timer_get_time() - last_sent_timestamp) > (CONFIG_MITHBRIDGE2_SEND_MAX_EMPTY_TIME_MINUTES * 60 * 1e6))
        {
            ESP_LOGE(tag, "No successfull sends during %d minutes, will restart", CONFIG_MITHBRIDGE2_SEND_MAX_EMPTY_TIME_MINUTES);
            esp_restart();
        }
        ESP_LOGI(tag, "Sending data complete");
    }
}

static void sensor_esp32_task(void *arg)
{
    static const char *tag = __func__;
    ESP_ERROR_CHECK(sensor_esp32_init());
    static sensor_data_t sd;
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000 * 15));
        ESP_ERROR_CHECK(sensor_esp32_fill(&sd));
        if (!sensor_data_collection_add((sensor_data_collection_t *)arg, &sd))
        {
            ESP_LOGD(tag, "Unable to add sensor data");
        }
    }
}