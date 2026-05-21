#include "led_flasher.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

#define LED_FLASHER_LED_ON (CONFIG_MITHBRIDGE2_LED_FLASHER_INVERTED ? 0 : 1)
#define LED_FLASHER_LED_OFF (CONFIG_MITHBRIDGE2_LED_FLASHER_INVERTED ? 1 : 0)

QueueHandle_t led_event_queue = NULL;

void led_flash_task(void *arg)
{
    uint8_t value;
    while (1)
    {
        if (xQueueReceive(led_event_queue, &value, portMAX_DELAY) == pdPASS)
        {
            gpio_set_level(CONFIG_MITHBRIDGE2_LED_FLASHER_LED_GPIO, LED_FLASHER_LED_ON);
            vTaskDelay(pdMS_TO_TICKS(CONFIG_MITHBRIDGE2_LED_FLASHER_ON_INTERVAL_MS));
            gpio_set_level(CONFIG_MITHBRIDGE2_LED_FLASHER_LED_GPIO, LED_FLASHER_LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(CONFIG_MITHBRIDGE2_LED_FLASHER_ON_INTERVAL_MS));
            gpio_set_level(CONFIG_MITHBRIDGE2_LED_FLASHER_LED_GPIO, LED_FLASHER_LED_ON);
            vTaskDelay(pdMS_TO_TICKS(CONFIG_MITHBRIDGE2_LED_FLASHER_ON_INTERVAL_MS));
            gpio_set_level(CONFIG_MITHBRIDGE2_LED_FLASHER_LED_GPIO, LED_FLASHER_LED_OFF);
            vTaskDelay(pdMS_TO_TICKS(CONFIG_MITHBRIDGE2_LED_FLASHER_OFF_INTERVAL_MS));
        }
    }
}

esp_err_t led_flasher_start(void)
{
    led_event_queue = xQueueCreate(CONFIG_MITHBRIDGE2_LED_FLASHER_QUEUE_SIZE, sizeof(uint8_t));
    if (led_event_queue == NULL)
    {
        return ESP_ERR_NO_MEM;
    }
    return xTaskCreate(led_flash_task, "led_flash_task", 1024, NULL, 5, NULL) == pdPASS ? ESP_OK : ESP_FAIL;
}

esp_err_t led_flasher_register_event()
{
    uint8_t value = 1;
    return (xQueueSendToFront(led_event_queue, &value, pdMS_TO_TICKS((CONFIG_MITHBRIDGE2_LED_FLASHER_OFF_INTERVAL_MS + CONFIG_MITHBRIDGE2_LED_FLASHER_ON_INTERVAL_MS) * 2)) == pdTRUE ? ESP_OK : ESP_FAIL);
}