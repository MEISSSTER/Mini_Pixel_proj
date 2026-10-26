#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// Пины экрана
#define PIN_TFT_CS    4
#define PIN_TFT_RST   5
#define PIN_TFT_DC    6
#define PIN_TFT_MOSI  7
#define PIN_TFT_SCLK  15

// Пины тачскрина (XPT2046)
#define PIN_TOUCH_MISO 16  // Подключить ТОЛЬКО к T_DO (MISO экрана не подключать!)
#define PIN_TOUCH_CS   17  // Подключить к T_CS

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9488 _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Touch_XPT2046 _touch_instance; // Добавляем объект тачскрина

public:
    LGFX(void) {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;
            cfg.spi_mode = 0;
            cfg.freq_write = 27000000;
            cfg.freq_read  = 16000000;
            cfg.spi_3wire  = false;
            cfg.use_lock   = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;

            cfg.pin_sclk = PIN_TFT_SCLK;
            cfg.pin_mosi = PIN_TFT_MOSI;
            cfg.pin_miso = PIN_TOUCH_MISO; // Указываем MISO для чтения тачскрина
            cfg.pin_dc   = PIN_TFT_DC;

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = PIN_TFT_CS;
            cfg.pin_rst          = PIN_TFT_RST;
            cfg.pin_busy         = -1;
            cfg.panel_width      = 320;
            cfg.panel_height     = 480;
            cfg.offset_x         = 0;
            cfg.offset_y         = 0;
            cfg.offset_rotation  = 0;
            cfg.readable         = false;
            cfg.invert           = false;
            cfg.rgb_order        = false;
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = true; // ВАЖНО: меняем на true, т.к. шина общая с тачем

            _panel_instance.config(cfg);
        }

        // Конфигурация тачскрина XPT2046
        {
            auto cfg = _touch_instance.config();
            cfg.x_min      = 300;  // Калибровочные минимумы и максимумы АЦП
            cfg.x_max      = 3900;
            cfg.y_min      = 300;
            cfg.y_max      = 3900;
            cfg.pin_int    = -1;   // T_IRQ не используем
            cfg.bus_shared = true; // Общая шина с экраном
            cfg.offset_rotation = 0;

            cfg.spi_host = SPI2_HOST; // Та же шина SPI2_HOST
            cfg.freq     = 1000000;   // Частота опроса тача 1 МГц
            cfg.pin_sclk = PIN_TFT_SCLK;
            cfg.pin_mosi = PIN_TFT_MOSI;
            cfg.pin_miso = PIN_TOUCH_MISO;
            cfg.pin_cs   = PIN_TOUCH_CS;

            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance); // Привязываем тач к панели
        }

        setPanel(&_panel_instance);
    }
};

LGFX tft;

void drawDashboardLayout() {
    tft.fillScreen(TFT_BLACK);

    tft.fillRect(0, 0, 480, 50, TFT_NAVY);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.setTextSize(2);
    tft.setCursor(20, 15);
    tft.println("ESP32-S3 System Monitor");

    tft.drawRect(20, 70, 440, 120, TFT_DARKGREY);
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(35, 90);
    tft.println("Status: Online");
    tft.setCursor(35, 120);
    tft.println("SPI Speed: 27 MHz");

    // Кнопка «START» (X: 20..220, Y: 220..290)
    tft.fillRoundRect(20, 220, 200, 70, 8, TFT_DARKGREEN);
    tft.drawRoundRect(20, 220, 200, 70, 8, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    tft.setTextSize(2);
    tft.setCursor(75, 245);
    tft.println("START");

    // Кнопка «RESET» (X: 260..460, Y: 220..290)
    tft.fillRoundRect(260, 220, 200, 70, 8, TFT_MAROON);
    tft.drawRoundRect(260, 220, 200, 70, 8, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_MAROON);
    tft.setTextSize(2);
    tft.setCursor(325, 245);
    tft.println("RESET");
}

void setup() {
    Serial.begin(115200);
    delay(500);

    tft.init();
    tft.setRotation(1);

    drawDashboardLayout();

    // Раскомментируйте эти 2 строки при первом запуске, чтобы точно откалибровать углы:
    // uint16_t calData[8];
    // tft.calibrateTouch(calData, TFT_WHITE, TFT_BLACK, 15);
}

unsigned int counter = 0;
unsigned long lastTimer = 0;
bool isRunning = true; // Флаг работы таймера для кнопки START

void refill_status_Uptime(unsigned int sec) {
    const int x = 35;
    const int y = 150;
    const int w = 400;
    const int h = 25;

    tft.fillRect(x, y, w, h, TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(x, y);
    tft.print("Up: ");

    unsigned int hours = sec / 3600;
    unsigned int minutes = (sec % 3600) / 60;
    unsigned int seconds = sec % 60;

    if (hours > 0) { tft.print(hours); tft.print("h "); }
    if (minutes > 0 || hours > 0) { tft.print(minutes); tft.print("m "); }
    tft.print(seconds);
    tft.print("s");
}

void loop() {
    // 1. Опрос тачскрина (выполняется постоянно без задержек)
    uint16_t touchX, touchY;
    if (tft.getTouch(&touchX, &touchY)) {
        Serial.printf("Touch: X=%d, Y=%d\n", touchX, touchY);

        // Проверка попадания в кнопку START (X от 20 до 220, Y от 220 до 290)
        if (touchX >= 20 && touchX <= 220 && touchY >= 220 && touchY <= 290) {
            isRunning = !isRunning; // Ставим на паузу или запускаем
            delay(250); // Защита от дребезга (повторного нажатия)
        }

        // Проверка попадания в кнопку RESET (X от 260 до 460, Y от 220 до 290)
        if (touchX >= 260 && touchX <= 460 && touchY >= 220 && touchY <= 290) {
            counter = 0;
            refill_status_Uptime(counter);
            delay(250);
        }
    }

    // 2. Обновление секундомера раз в 1000 мс без блокировки программы
    if (isRunning && (millis() - lastTimer >= 1000)) {
        lastTimer = millis();
        refill_status_Uptime(counter);
        counter++;
    }
}