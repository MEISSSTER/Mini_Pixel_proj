#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lvgl.h"
#include "pin_config.h"

#define TFT_HOR_RES 320
#define TFT_VER_RES 480

// bool lvgl_init(){
//     lv_display_t * disp = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
//     lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
//     lv_display_set_flush_cb(disp, flush_cb);

//     lv_indev_t * indev = lv_indev_create();
//     lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
//     lv_indev_set_read_cb(indev, read_cb);

//     return true;

// }


extern "C" void app_main(void) {

    lv_init();

    while(1){
        lv_timer_handler();
        vTaskDelay(1);

    }
}
