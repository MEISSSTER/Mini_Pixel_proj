#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h" // Основная библиотека для работы с пинами
#include "Stepmotor.h"

#define LED_PIN GPIO_NUM_44  // пин, к которому подключен светодиод
void app_main(void) {
    StepMotor stepMotor;
    stepMotor.begin();
    stepMotor.enable();
  

    while(1) {
        stepMotor.move(200, DIR_UP);  // Двигаем вверх на 200 шагов
        stepMotor.move(100, DIR_DOWN); // Двигаем вниз на 100 шагов
        // 1. Сброс и настройка пина
        gpio_reset_pin(LED_PIN);
        gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    }
}
