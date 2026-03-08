#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_PIN  44 
#define STEP_PIN 1
#define DIR_PIN  2

void app_main(void) {
    gpio_reset_pin(LED_PIN);

    gpio_reset_pin(STEP_PIN);
    gpio_set_direction(STEP_PIN, GPIO_MODE_OUTPUT);

    gpio_reset_pin(DIR_PIN);
    gpio_set_direction(DIR_PIN, GPIO_MODE_OUTPUT);

    while(1) {
        printf("Step\n");
        gpio_set_level(DIR_PIN, 1);
        gpio_set_level(LED_PIN, 1);
        gpio_set_level(STEP_PIN, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(LED_PIN, 0);
        gpio_set_level(STEP_PIN, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        printf("Stepback\n");
        gpio_set_level(DIR_PIN, 0);
        gpio_set_level(LED_PIN, 1);
        gpio_set_level(STEP_PIN, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(STEP_PIN, 0);
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    }
        
}