#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

// ─── Пины A4988 ───────────────────────────────────────────────
#define PIN_STEP        GPIO_NUM_1   // STEP  → A4988
#define PIN_DIR         GPIO_NUM_2   // DIR   → A4988
#define PIN_EN          GPIO_NUM_3   // EN    → A4988 (LOW = вкл)
#define PIN_ENDSTOP     GPIO_NUM_4   // Концевой выключатель (LOW = нажат)

// ─── Параметры движения ───────────────────────────────────────
#define STEP_DELAY_US        800     // Задержка между импульсами (мкс) — скорость
#define HOMING_STEP_DELAY_US 1500   // Скорость при калибровке (медленнее)
#define STEPS_PER_MM         80     // Шагов на 1 мм (полный шаг, 1/1)
#define BACKOFF_STEPS        200    // Отъезд от концевика после homing (шагов)
#define MAX_TRAVEL_STEPS     50000  // Макс. ход (защита от выезда за пределы)

// ─── Параметры цикла печати слоя ──────────────────────────────
// Последовательность на каждый слой:
//   1. Подъём на (PEEL_LIFT_MM + layer_height) — отрыв от FEP плёнки
//   2. Опускание на PEEL_LIFT_MM — возврат к позиции экспозиции
//   3. Засветка ультрафиолетом
#define PEEL_LIFT_MM         5.0f   // Высота дополнительного подъёма для отрыва (мм)

// ─── Направления ──────────────────────────────────────────────
#define DIR_UP    1
#define DIR_DOWN  0

static const char* TAG_MOTOR = "StepMotor";

/**
 * @brief Класс управления шаговым двигателем NEMA17 через драйвер A4988
 *
 * Логика:
 *  - EN активен LOW (мотор держит позицию при LOW, отпускает при HIGH)
 *  - Каждый импульс на STEP = 1 шаг
 *  - DIR определяет направление
 *  - Концевик подтянут к VCC, при нажатии — GND (LOW)
 */

class StepMotor 
{
public:
    StepMotor() : _position(0), _isHomed(false) {}

    /**
     * @brief Инициализация GPIO
     */
    void begin() {
        // Настройка выходов
        gpio_config_t out_cfg = {};
        out_cfg.pin_bit_mask = (1ULL << PIN_STEP) |
                               (1ULL << PIN_DIR)  |
                               (1ULL << PIN_EN);
        out_cfg.mode         = GPIO_MODE_OUTPUT;
        out_cfg.pull_up_en   = GPIO_PULLUP_DISABLE;
        out_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        out_cfg.intr_type    = GPIO_INTR_DISABLE;
        gpio_config(&out_cfg);

        // Настройка входа концевика (подтяжка к VCC)
        gpio_config_t in_cfg = {};
        in_cfg.pin_bit_mask  = (1ULL << PIN_ENDSTOP);
        in_cfg.mode          = GPIO_MODE_INPUT;
        in_cfg.pull_up_en    = GPIO_PULLUP_ENABLE;   // нет внешней подтяжки — используем внутреннюю
        in_cfg.pull_down_en  = GPIO_PULLDOWN_DISABLE;
        in_cfg.intr_type     = GPIO_INTR_DISABLE;
        gpio_config(&in_cfg);

        disable();  // Мотор выключен до homing
        gpio_set_level(PIN_STEP, 0);
        gpio_set_level(PIN_DIR, DIR_UP);

        ESP_LOGI(TAG_MOTOR, "StepMotor initialized");
    }

    /**
     * @brief Включить мотор (держит позицию)
     */
    void enable() {
        gpio_set_level(PIN_EN, 0);  // EN = LOW → активен
    }

    /**
     * @brief Выключить мотор (катушки обесточены)
     */
    void disable() {
        gpio_set_level(PIN_EN, 1);  // EN = HIGH → отключён
    }

    /**
     * @brief Сделать N шагов в заданном направлении
     * @param steps     Количество шагов
     * @param direction DIR_UP или DIR_DOWN
     * @param delay_us  Задержка между шагами (мкс)
     * @return true если движение выполнено, false если достигнут концевик (при движении вниз)
     */
    bool move(int32_t steps, uint8_t direction, uint32_t delay_us = STEP_DELAY_US) {
        if (steps <= 0) return true;

        gpio_set_level(PIN_DIR, direction);
        esp_rom_delay_us(2);  // Минимальная выдержка после смены DIR (A4988: ≥200нс)

        for (int32_t i = 0; i < steps; i++) {
            // Проверка концевика при движении вниз
            if (direction == DIR_DOWN && isEndstopTriggered()) {
                ESP_LOGW(TAG_MOTOR, "Endstop hit after %ld steps", i);
                _position = 0;
                return false;
            }

            // Генерация импульса STEP (A4988: HIGH ≥1мкс, LOW ≥1мкс)
            gpio_set_level(PIN_STEP, 1);
            esp_rom_delay_us(2);
            gpio_set_level(PIN_STEP, 0);
            esp_rom_delay_us(delay_us);

            // Обновление позиции
            if (direction == DIR_UP)   _position++;
            else                       _position--;
        }
        return true;
    }

    /**
     * @brief Переместить на заданное количество миллиметров
     * @param mm        Расстояние в мм (+ = вверх, - = вниз)
     */
    bool moveMM(float mm) {
        if (!_isHomed) {
            ESP_LOGE(TAG_MOTOR, "Not homed! Call homing() first.");
            return false;
        }
        int32_t steps = (int32_t)(mm * STEPS_PER_MM);
        uint8_t dir   = (steps > 0) ? DIR_UP : DIR_DOWN;
        return move(abs(steps), dir);
    }

    /**
     * @brief Калибровка нулевой позиции (Homing)
     *
     * Алгоритм:
     *  1. Едем вниз до срабатывания концевика
     *  2. Немного откатываемся вверх (backoff)
     *  3. Медленно снова вниз до концевика (точная позиция)
     *  4. Откатываемся на BACKOFF_STEPS — это рабочий Z=0
     */
    bool homing() {
        ESP_LOGI(TAG_MOTOR, "Starting homing...");
        enable();

        // Фаза 1: Быстрый спуск до концевика
        ESP_LOGI(TAG_MOTOR, "Phase 1: Fast descent...");
        bool endstopHit = false;
        for (int32_t i = 0; i < MAX_TRAVEL_STEPS; i++) {
            if (isEndstopTriggered()) {
                endstopHit = true;
                break;
            }
            gpio_set_level(PIN_DIR, DIR_DOWN);
            esp_rom_delay_us(2);
            gpio_set_level(PIN_STEP, 1);
            esp_rom_delay_us(2);
            gpio_set_level(PIN_STEP, 0);
            esp_rom_delay_us(HOMING_STEP_DELAY_US);
        }

        if (!endstopHit) {
            ESP_LOGE(TAG_MOTOR, "Homing failed: endstop not reached!");
            disable();
            return false;
        }

        // Фаза 2: Откат вверх
        ESP_LOGI(TAG_MOTOR, "Phase 2: Backoff...");
        move(BACKOFF_STEPS, DIR_UP, HOMING_STEP_DELAY_US);

        // Фаза 3: Медленный спуск до концевика (точная позиция)
        ESP_LOGI(TAG_MOTOR, "Phase 3: Slow precise descent...");
        for (int32_t i = 0; i < BACKOFF_STEPS * 2; i++) {
            if (isEndstopTriggered()) break;
            gpio_set_level(PIN_DIR, DIR_DOWN);
            esp_rom_delay_us(2);
            gpio_set_level(PIN_STEP, 1);
            esp_rom_delay_us(2);
            gpio_set_level(PIN_STEP, 0);
            esp_rom_delay_us(HOMING_STEP_DELAY_US * 2);  // Ещё медленнее
        }

        // Фаза 4: Финальный откат — это Z=0
        move(BACKOFF_STEPS, DIR_UP, HOMING_STEP_DELAY_US);

        _position = 0;
        _isHomed  = true;
        ESP_LOGI(TAG_MOTOR, "Homing complete. Z=0 set.");
        return true;
    }

    /**
     * @brief Полный цикл движения для одного слоя (peel sequence)
     *
     * Последовательность:
     *   1. Подъём на (PEEL_LIFT_MM + layer_height_mm) — отрывает слой от FEP плёнки
     *   2. Опускание на PEEL_LIFT_MM — возвращает платформу на высоту следующей экспозиции
     *   После возврата вызывающий код должен выполнить UV-засветку.
     *
     * @param layer_height_mm  Высота слоя в мм
     * @return true если оба движения выполнены успешно
     */
    bool printLayerSequence(float layer_height_mm) {
        if (!_isHomed) {
            ESP_LOGE(TAG_MOTOR, "Not homed! Call homing() first.");
            return false;
        }

        float lift_total = PEEL_LIFT_MM + layer_height_mm;

        // Шаг 1: Подъём вверх на (5мм + высота слоя)
        ESP_LOGI(TAG_MOTOR, "Peel lift: +%.3f mm (5.0 peel + %.3f layer)",
                 lift_total, layer_height_mm);
        if (!moveMM(lift_total)) {
            ESP_LOGE(TAG_MOTOR, "Lift failed!");
            return false;
        }

        // Шаг 2: Опускание вниз на 5мм — платформа остаётся на высоте слоя
        ESP_LOGI(TAG_MOTOR, "Retract: -%.3f mm", PEEL_LIFT_MM);
        if (!moveMM(-PEEL_LIFT_MM)) {
            ESP_LOGE(TAG_MOTOR, "Retract failed!");
            return false;
        }

        ESP_LOGI(TAG_MOTOR, "Layer sequence done. Z = %.3f mm", getPositionMM());
        return true;

        // → Вызывающий код выполняет UV-экспозицию (Сделать в другом заголовочном файле вместе с выводом изображения на экран)
    }

    /**
     * @brief Проверить состояние концевика
     * @return true если нажат
     */
    bool isEndstopTriggered() const {
        return gpio_get_level(PIN_ENDSTOP) == 0;  // LOW = нажат
    }

    /**
     * @brief Текущая позиция в шагах
     */
    int32_t getPositionSteps() const { return _position; }

    /**
     * @brief Текущая позиция в мм
     */
    float getPositionMM() const { return (float)_position / STEPS_PER_MM; }

    /**
     * @brief Была ли выполнена калибровка
     */
    bool isHomed() const { return _isHomed; }

private:
    int32_t _position;  // Текущая позиция в шагах (0 = Z=0 после homing)
    bool    _isHomed;   // Флаг: калибровка выполнена
};