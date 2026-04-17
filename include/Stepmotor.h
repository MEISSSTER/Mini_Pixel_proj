#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// Пины драйвера
#define PIN_STEP GPIO_NUM_1    // Пин для импульса
#define PIN_DIR GPIO_NUM_2     // Пин для направления
#define PIN_ENABLE GPIO_NUM_3  // Пин для включения

// Пин концевика
#define PIN_ENDSTOP GPIO_NUM_4

// Параметры движения
#define STEP_DELAY 800        // Задержка импульсов в мкс
#define HOMING_STEP_DELAY 1500 // Задержка импульсов при калибровке в мкс
#define STEPS_PER_MM 80       // Шагов на 1мм
#define BACKOFF_STEPS 200     // Выход на рабочий 0
#define MAX_STEPS 50000       // Максимально кол-во шагов (Защита от выхода)
#define LIFT_MM 5.0f          // Высота для отрыва
#define DIR_UP 1              // Направление вниз 
#define DIR_DOWN 0            // Направление вниз

static const char* TAG_MOTOR = "StepMotor";

/** 
*   @brief Класс управления шаговым длигателем NEMA17
*   Логика печати слоя:
*       1. Подъем на LIFT_MM + layer_height
*       2. Опускание на layer_height
*       3. Засвет

*   Логика управления двигателем:
*      1. ENABLE активен при LOW (Мотор держит позицию - LOW, отпускает при HIGH) 
*      2. Каждый импуль на STEP = шаг
*      3. DIR - направление
*      4. Концевик с внутренней подтяжкой, нажатие = LOW 
*/

class StepMotor
{
public:
    StepMotor() : _position(0), _isHomed(false) {}
    /**
     * @brief Инициализация пинов
     */
    void begin()
    {
        gpio_config_t out_cfg = {};
        out_cfg.pin_bit_mask =  (1ULL << PIN_STEP) | 
                                (1ULL << PIN_DIR)  | 
                                (1ULL << PIN_ENABLE);
        out_cfg.mode         = GPIO_MODE_OUTPUT;
        out_cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
        out_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        out_cfg.intr_type    = GPIO_INTR_DISABLE;

        gpio_config_t in_cfg = {};
        in_cfg.pin_bit_mask  = (1ULL << PIN_ENDSTOP);
        in_cfg.mode          = GPIO_MODE_INPUT;
        in_cfg.pull_up_en    = GPIO_PULLUP_ENABLE;
        in_cfg.pull_down_en  = GPIO_PULLDOWN_DISABLE;
        in_cfg.intr_type     = GPIO_INTR_DISABLE;

        gpio_config(&out_cfg);
        gpio_config(&in_cfg);

        disable();
        gpio_set_level(PIN_STEP, 0);
        gpio_set_level(PIN_DIR, DIR_UP);

        ESP_LOGI(TAG_MOTOR, "StepMotor initialized");
    }

    /**
     * @brief Включить мотор (Держит позицию)
     */
    void enable()
    {
        gpio_set_level(PIN_ENABLE, 0);
    }

    /**
     * @brief Выключить мотор (Обесточить катушки. отпускает позицию)
     */
    void disable()
    {
        gpio_set_level(PIN_ENABLE, 1);
    }

    /**
     * @brief Сделать заданное количество шагов в заданном направлении
     * @param steps Количество шагов
     * @param direction DIR_UP - Вверх, DIR_DOWN - Вниз
     * @param delay_us Задержка между шагами в мкс. Задана по умолчанию
     * @return true если движение выполнено успешно, false если достигнут концевик
     */
    bool move(int32_t steps, uint8_t direction, uint32_t delay_us = STEP_DELAY)
    {
        if (steps <= 0) return true;

        gpio_set_level(PIN_DIR, direction);
        esp_rom_delay_us(2);

        for (int32_t i = 0; steps > i; i++)
        {
            if (direction == DIR_DOWN && isEndstopTriggered())
            {
                ESP_LOGW(TAG_MOTOR, "endstop is triggered after %ld steps", i);
                _position = 0;
                return false;
            }

            gpio_set_level(PIN_STEP, 1);
            esp_rom_delay_us(2);
            gpio_set_level(PIN_STEP, 0);
            esp_rom_delay_us(delay_us);

            if (direction == DIR_UP)  _position++;
            else                      _position--;
        }
        return true;
    }

    /**
     * @brief Переместить платформу на заданное количество миллиметров в заданном направлении
     * @param mm Расстояние в миллиметрах
     * @param direction DIR_UP - Вверх, DIR_DOWN - Вниз.
     */
    bool moveMM(float mm, uint8_t direction)
    {
        if (!_isHomed)
        {
            ESP_LOGE(TAG_MOTOR, "Not homed! Call homing() first.");
            return false;
        }

        int32_t steps = (int32_t)(mm * STEPS_PER_MM);
        return move(abs(steps), direction);
    }

    /**
     * @brief Калибровка двигателя до нулевой позиции
     * Реализация через концевик в виде кнопки
     * Алгоритм:
     *      1. Спуск до кнопки со скорость калибровки
     *      2. ОПЦИОНАЛЬНО!!! Откат на рабочий 0
     */
    bool homing()
    {
        enable();
        bool endstopHit = false;

        // Фаза 1
        for (int32_t i = 0; i < MAX_STEPS; i++)
        {
            if (isEndstopTriggered())
            {
                endstopHit = true;
                break;
            }

            gpio_set_level(PIN_DIR, DIR_DOWN);
            esp_rom_delay_us(2);
            gpio_set_level(PIN_STEP, 1);
            esp_rom_delay_us(2);
            gpio_set_level(PIN_STEP, 0);
            esp_rom_delay_us(HOMING_STEP_DELAY);
        }

        if (!endstopHit)
        {
            ESP_LOGE(TAG_MOTOR, "Homing failed: endstop not reached!");
            disable();
            return false;
        }

        // Фаза 2 - ОПЦИОНАЛЬНО
        // move(BACKOFF_STEPS, DIR_UP);

        _position = 0;
        _isHomed = true;
        ESP_LOGI(TAG_MOTOR, "Homing complete. Z=0 set.");
        return true;
    }

    /**
     * @brief Полный цикл движения двигателя для отпечатки слоя
     * 
     *   Последовательность:
     *      1. Подъём на (LIFT_MM + layer_height_mm) — отрывает слой от FEP плёнки
     *      2. Опускание на LIFT_MM — возвращает платформу на высоту следующей экспозиции
     *      3. Засветка слоя
     * 
     * @param layer_height_mm  Высота слоя в мм
     * @return true если оба движения выполнены успешно
     */
    bool printLayerSequence(float layer_height_mm) 
    {
        if (!_isHomed) 
        {
            ESP_LOGE(TAG_MOTOR, "Not homed! Call homing() first.");
            return false;
        }

        float lift_total = LIFT_MM + layer_height_mm;

        // Фаза 1
        if (!moveMM(lift_total, DIR_UP))
        {
            ESP_LOGE(TAG_MOTOR, "Lift failed!");
            return false;
        }

        // Фаза 2
        if (!moveMM(LIFT_MM, DIR_DOWN))
        {
            ESP_LOGE(TAG_MOTOR, "Retract failed!");
            return false;
        }
        
        // Фаза 3
        ESP_LOGI(TAG_MOTOR, "Layer sequence done. Z = %.3f mm", getPositionMM());
        return true; 
        // Вызывающий код выполняет UV-экспозицию
    }
    
    /**
     * @brief Проверить состояние концевика
     * @return true если нажат
     */
    bool isEndstopTriggered() const
    {
        return gpio_get_level(PIN_ENDSTOP) == 0;
    }

    /**
     * @brief Текущая позиция в шагах
     */
    int32_t getPositionSteps() const
    {
        return _position;
    }

    /**
     * @brief Текущая позиция в мм
     */
    int32_t getPositionMM() const
    {
        return (float)_position / STEPS_PER_MM;
    }

    /**
     * @brief Была ли выполнена калибровка
     */
    bool isHomed() const
    {
        return _isHomed;
    }

private:
    int32_t _position; // Текущая позиция в шагах
    bool _isHomed; // Флаг калибровки двигателя
};