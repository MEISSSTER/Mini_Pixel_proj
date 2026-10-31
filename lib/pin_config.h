#pragma once 
#include <Arduino.h>

// ==========================================
// ЯДРО 1: Первый экран (GUI) + Тачскрин
// Шина: SPI3_HOST
// ==========================================
constexpr uint8_t PIN_TFT0_CS   = 4;
constexpr uint8_t PIN_TFT0_RST  = 5;
constexpr uint8_t PIN_TFT0_DC   = 6;
constexpr uint8_t PIN_TFT0_MOSI = 7;
constexpr uint8_t PIN_TFT0_SCLK = 15;

constexpr uint8_t PIN_TOUCH_MISO = 16;
constexpr uint8_t PIN_TOUCH_CS   = 17;


// ==========================================
// ЯДРО 0: Второй экран (УФ-Маска) + SD-карта
// Шина: SPI2_HOST (Общая для экрана и флешки)
// ==========================================
// Базовые пины шины SPI2 (Физически общие провода!)
constexpr uint8_t PIN_SPI2_SCLK = 12;
constexpr uint8_t PIN_SPI2_MOSI = 11;
constexpr uint8_t PIN_SPI2_MISO = 13; // Нужен только для SD-карты

// Пины для второго экрана (Маска)
constexpr uint8_t PIN_TFT1_SCLK = PIN_SPI2_SCLK; // 12
constexpr uint8_t PIN_TFT1_MOSI = PIN_SPI2_MOSI; // 11
constexpr uint8_t PIN_TFT1_CS   = 10;
constexpr uint8_t PIN_TFT1_DC   = 9;
constexpr uint8_t PIN_TFT1_RST  = 8;
// У маски MISO не подключается (-1 в конфигурации LovyanGFX)

// Пины для SD-карты
constexpr uint8_t PIN_SD_SCLK   = PIN_SPI2_SCLK; // 12
constexpr uint8_t PIN_SD_MOSI   = PIN_SPI2_MOSI; // 11
constexpr uint8_t PIN_SD_MISO   = PIN_SPI2_MISO; // 13
constexpr uint8_t PIN_SD_CS     = 14;