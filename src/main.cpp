#include <Arduino.h>
#include "screen_classes.hpp"
#include "pin_config.h"

LGFX tftf;
Draw_interface screen(tftf); // Создали глобально 

void setup() {
    Serial.begin(115200);
    delay(500);

    // ВАЖНО: Инициализируем дисплей перед отрисовкой
    tftf.init();
    tftf.setRotation(1);

    screen.drawDashboardLayout(); // Отрисовали
}

void loop() {
    uint8_t btn = screen.button_istouched();
    
    if (btn != 254 && btn != 255) {
        Serial.print("Pressed button ID: ");
        Serial.println(btn);
        delay(200); // Базовый антидребезг
    }
}