#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include "pin_config.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>


// --- БАЗОВЫЙ УНИВЕРСАЛЬНЫЙ КЛАСС ЭКРАНА ---
class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9488 _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Touch_XPT2046 _touch_instance;

public:
    LGFX(spi_host_device_t host, int sck, int mosi, int miso, int cs, int dc, int rst, int touch_cs, bool shared) {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = host; // Используем переданную шину, а не жестко SPI2
            cfg.spi_mode = 0;
            cfg.freq_write = 27000000;
            cfg.freq_read  = 16000000;
            cfg.spi_3wire  = false;
            cfg.use_lock   = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;

            cfg.pin_sclk = sck;
            cfg.pin_mosi = mosi;
            cfg.pin_miso = miso;
            cfg.pin_dc   = dc;

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = cs;
            cfg.pin_rst          = rst;
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
            cfg.bus_shared       = shared;

            _panel_instance.config(cfg);
        }
        
        // Включаем тачскрин, только если передан реальный пин
        if (touch_cs >= 0) {
            auto cfg = _touch_instance.config();
            cfg.x_min      = 300; 
            cfg.x_max      = 3900;
            cfg.y_min      = 300;
            cfg.y_max      = 3900;
            cfg.pin_int    = -1;  
            cfg.bus_shared = shared; 
            cfg.offset_rotation = 0;

            cfg.spi_host = host; // Тот же host, что и у дисплея
            cfg.freq     = 1000000;   
            cfg.pin_sclk = sck;
            cfg.pin_mosi = mosi;
            cfg.pin_miso = miso;
            cfg.pin_cs   = touch_cs;

            _touch_instance.config(cfg);
            _panel_instance.setTouch(&_touch_instance); 
        }

        setPanel(&_panel_instance);
    }
};

// --- КЛАСС ИНТЕРФЕЙСА (UI) ---
class Draw_interface {
private:
    LGFX& tft; // Ссылка на экран
    static constexpr int buttonWidth = 140;
    static constexpr int buttonHeight = 70;
    static constexpr int buttonGap = 10;
    static constexpr int groupWidth = buttonWidth * 3 + buttonGap * 2;

public:
    struct Button_Properties{
        uint16_t x, y, width, height;
        uint8_t id;
    };
    static const uint8_t n_buttons = 3;
    Button_Properties active_buttons[n_buttons]; 

    // Конструктор просто сохраняет ссылку
    Draw_interface(LGFX& display_ref) : tft(display_ref) {}

    void drawDashboardLayout(int buttonsX = -1, int buttonsY = 220) {
        tft.fillScreen(TFT_BLACK);

        tft.fillRect(0, 0, tft.width(), 50, TFT_NAVY);
        tft.setTextColor(TFT_WHITE, TFT_NAVY);
        tft.setTextSize(2);
        tft.setCursor(20, 15);
        tft.println("MiniPixel Interface Ver 0.1");

        const int groupX = buttonsX < 0 ? (tft.width() - groupWidth) / 2 : buttonsX;
        const char* labels[] = {"START", "STOP", "DEBUG"};
        const uint16_t colors[] = {TFT_DARKGREEN, TFT_MAROON, TFT_NAVY};

        for (int i = 0; i < 3; ++i) {
            const int x = groupX + i * (buttonWidth + buttonGap);
            tft.fillRoundRect(x, buttonsY, buttonWidth, buttonHeight, 8, colors[i]);
            tft.drawRoundRect(x, buttonsY, buttonWidth, buttonHeight, 8, TFT_WHITE);
            tft.setTextColor(TFT_WHITE, colors[i]);
            tft.setTextSize(2);
            tft.setCursor(x + (buttonWidth - tft.textWidth(labels[i])) / 2,
                          buttonsY + (buttonHeight - 16) / 2);
            tft.println(labels[i]);

            active_buttons[i] = {(uint16_t)x, (uint16_t)buttonsY, buttonWidth, buttonHeight, (uint8_t)i};
        }
    }

    uint8_t button_istouched(){
        uint16_t Pos_x, Pos_y;
        if(tft.getTouch(&Pos_x, &Pos_y)){
            for (int i = 0; i<n_buttons; i++){
                if (Pos_x > active_buttons[i].x && Pos_x < (active_buttons[i].x + active_buttons[i].width) && 
                    Pos_y > active_buttons[i].y && Pos_y < (active_buttons[i].y + active_buttons[i].height)){
                    return active_buttons[i].id;
                }
            }
            return 255; // Нажатие мимо кнопок
        }
        return 254; // Нет нажатия
    }
};

// --- КЛАСС ЭКРАНА МАСКИ ---
class Mask_Screen {
private:
    LGFX& tft; // Привязали экран один раз при создании

public:
    Mask_Screen(LGFX& display_ref) : tft(display_ref) {}

    // 1. Очистка экрана (заливка черным, чтобы смола не засвечивалась зря)
    void clear() {
        tft.fillScreen(TFT_BLACK);
    }

    // 2. Вывод готового кадра из буфера в оперативке (ОЗУ)
    // Передаем указатель на массив пикселей/байтов
    void drawFromBuffer(const uint8_t* image_buffer, size_t size) {
        // Если храним в ОЗУ сжатые PNG или BMP:
        tft.drawBmp(image_buffer, size, 0, 0); 
        // Или если храним сырой битовый массив (1 бит = 1 пиксель):
        // tft.drawBitmap(0, 0, image_buffer, 320, 480, TFT_WHITE, TFT_BLACK);
    }

    // 3. Прямой вывод с SD-карты (пригодится для быстрых тестов без буфера!)
    void drawDirectlyFromSD(const char* filepath) {
        tft.drawBmpFile(SD, filepath, 0, 0);
    }
};



