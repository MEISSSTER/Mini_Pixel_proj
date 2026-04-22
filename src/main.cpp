#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "pin_config.h"

#define TFT_HOR_RES 320
#define TFT_VER_RES 480



extern "C" void app_main(void) {

    lv_init();

    while(1){
        lv_timer_handler();
        vTaskDelay(1);

    }
};
