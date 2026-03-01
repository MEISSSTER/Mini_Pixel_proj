#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h" // Основная библиотека для работы с пинами

#define LED_PIN 44 // Номер встроенного светодиода

void app_main(void) {
    // 1. Сброс и настройка пина
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    while(1) {
        // 2. Включить/выключить (1 или 0)
        printf("LED ON\n");
        gpio_set_level(LED_PIN, 1);
        
        // 3. Задержка (в FreeRTOS измеряется в тиках)
        vTaskDelay(1000 / portTICK_PERIOD_MS); 

        printf("LED OFF\n");
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
