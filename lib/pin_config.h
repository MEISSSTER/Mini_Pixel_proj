#pragma once // Защита от двойного включения
#include <Arduino.h>

// Лучше использовать constexpr вместо #define для строгой типизации
constexpr uint8_t PIN_TFT_CS   = 4;
constexpr uint8_t PIN_TFT_RST  = 5;
constexpr uint8_t PIN_TFT_DC   = 6;
constexpr uint8_t PIN_TFT_MOSI = 7;
constexpr uint8_t PIN_TFT_SCLK = 15;

constexpr uint8_t PIN_TOUCH_MISO = 16;
constexpr uint8_t PIN_TOUCH_CS   = 17;