#include "sdkconfig.h"
#include "esp_err.h"
#include "esp_console.h"
#include "nvs_keys.h"
#include "configurator.h"
#include "argtable3/argtable3.h"
#include "string.h"
#include "esp_log.h"

static const char *MITHBRIDGE2_CONSOLE_GETSET_OP_SET = "set";
static const char *MITHBRIDGE2_CONSOLE_GETSET_OP_GET = "get";

static const char *set_params[] = {MITHBRIDGE2_NVS_WIFI_SSID_KEY, MITHBRIDGE2_NVS_WIFI_PSK_KEY, MITHBRIDGE2_NVS_INFLUX_LINE_HTTPS_AUTH_KEY, MITHBRIDGE2_NVS_INFLUX_LINE_HTTPS_URL_KEY};

char *getset_params_datatype()
{
    size_t item_count = (sizeof(set_params) / sizeof(set_params[0]));
    size_t buf_size = 1 /* \0*/ + (item_count - 1) /*delims*/;
    for (size_t i = 0; i < item_count; i++)
    {
        buf_size += strlen(set_params[i]);
    }
    char *str = malloc(buf_size);
    char *p = str;
    for (size_t i = 0; i < item_count; i++)
    {
        p += sprintf(p, "%s|", set_params[i]);
    }
    p--;
    return str;
}

int configurator_console_handle_cmd_restart(int argc, char **argv)
{
    esp_restart();
    return 0;
}

int configurator_console_handle_cmd_getset(void *context, int argc, char **argv)
{
    if (((const char *)context) == MITHBRIDGE2_CONSOLE_GETSET_OP_GET && argc == 2)
    {
        char *val = (char *)configurator_get_str_alloc(argv[1]);
        if (val == NULL)
        {
            printf("# Parameter '%s' not set in configuration\n", argv[1]);
        }
        else
        {
            printf("# READ:  '%s' = '%s'\n", argv[1], val);
            free((void *)val);
        }
    }
    else if (((const char *)context) == MITHBRIDGE2_CONSOLE_GETSET_OP_SET && argc == 3)
    {
        const char *key = NULL;
        for (size_t i = 0; i < (sizeof(set_params) / sizeof(set_params[0])); i++)
        {
            if (strcasecmp(argv[1], set_params[i]) == 0)
            {
                key = set_params[i];
                break;
            }
        }
        if (key != NULL)
        {
            configurator_set_str(key, argv[2]);
            printf("# SAVED: '%s' configuration\n", argv[1]);
        }
        else
        {
            printf("! Parameter '%s' not supported by firmware\n", argv[1]);
        }
    }
    else
    {
        printf("#! Unsupported command or missing args");
    }
    printf("\n");
    return 0;
}

void configurator_console_run()
{
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    esp_console_dev_usb_serial_jtag_config_t dev_cfg = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    repl_config.prompt = "config> ";
    repl_config.max_cmdline_length = 1024;

    setvbuf(stdin, NULL, _IONBF, 0);

    void *set_argtable[] = {
        arg_str1(NULL, NULL, getset_params_datatype(), "param name"),
        arg_str1(NULL, NULL, "\"value\"", "param value"),
        arg_end(20),
    };

    void *get_argtable[] = {
        arg_str1(NULL, NULL, getset_params_datatype(), "param name"),
        arg_end(20),
    };

    esp_console_cmd_t cmd_set = {
        .command = "set",
        .help = "set parameter and save to flash",
        .func_w_context = configurator_console_handle_cmd_getset,
        .context = (void *)MITHBRIDGE2_CONSOLE_GETSET_OP_SET,
        .func = NULL,
        .argtable = set_argtable,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_set));

    esp_console_cmd_t cmd_get = {
        .command = "get",
        .help = "get parameter from flash",
        .func_w_context = configurator_console_handle_cmd_getset,
        .context = (void *)MITHBRIDGE2_CONSOLE_GETSET_OP_GET,
        .func = NULL,
        .argtable = get_argtable,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_get));

    esp_console_cmd_t cmd_restart = {
        .command = "restart",
        .help = "restart",
        .context = NULL,
        .func = configurator_console_handle_cmd_restart,
        .func_w_context = NULL,
        .argtable = NULL,
    };

    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&dev_cfg, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_cmd_register(&cmd_restart));
    ESP_ERROR_CHECK(esp_console_set_help_verbose_level(ESP_CONSOLE_HELP_VERBOSE_LEVEL_MAX_NUM - 1));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));

    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}