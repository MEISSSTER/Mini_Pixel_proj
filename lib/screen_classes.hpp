#pragma once
#include <Arduino.h>
#include "pin_config.h"
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
        lgfx::Panel_ILI9488 _panel_instance;
        lgfx::Bus_SPI       _bus_instance;
        lgfx::Touch_XPT2046 _touch_instance;

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
                cfg.pin_miso = PIN_TOUCH_MISO;
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
                cfg.bus_shared       = true;

                _panel_instance.config(cfg);
            }

            // Конфигурация тачскрина XPT2046
            {
                auto cfg = _touch_instance.config();
                cfg.x_min      = 300; 
                cfg.x_max      = 3900;
                cfg.y_min      = 300;
                cfg.y_max      = 3900;
                cfg.pin_int    = -1;  
                cfg.bus_shared = true; 
                cfg.offset_rotation = 0;

                cfg.spi_host = SPI2_HOST; 
                cfg.freq     = 1000000;   
                cfg.pin_sclk = PIN_TFT_SCLK;
                cfg.pin_mosi = PIN_TFT_MOSI;
                cfg.pin_miso = PIN_TOUCH_MISO;
                cfg.pin_cs   = PIN_TOUCH_CS;

                _touch_instance.config(cfg);
                _panel_instance.setTouch(&_touch_instance); 
            }

            setPanel(&_panel_instance);
        } // 1. ЗДЕСЬ ЗАКРЫВАЕТСЯ КОНСТРУКТОР LGFX(void)
}; // 2. ЗДЕСЬ ЗАКРЫВАЕТСЯ КЛАСС LGFX



class Draw_interface {
private:
    // 3. Теперь мы можем спокойно создать объект экрана
    LGFX& tft; // Передаём указатель на экземпляр класса
    static constexpr int buttonWidth = 140;
    static constexpr int buttonHeight = 70;
    static constexpr int buttonGap = 10;
    static constexpr int groupWidth = buttonWidth * 3 + buttonGap * 2;

public:
    // 4. КОНСТРУКТОР ГЛАВНОГО КЛАССА: здесь мы выполняем исполняемый код (init и setRotation)
    Draw_interface(LGFX& display_ref) : tft(display_ref){

    }

    struct Button_Properties{
        uint16_t x;
        uint16_t y;
        uint16_t width;
        uint16_t height;
        uint8_t id;
    };
    static const uint8_t n_buttons = 3; /// Указанное значение выражает кол-во активных кнопок, в случае изменения кол-ва -- поправить
    Button_Properties active_buttons[n_buttons]; 

    void drawDashboardLayout(int buttonsX = -1, int buttonsY = 220) {
        
        tft.fillScreen(TFT_BLACK);

        // Шапка
        tft.fillRect(0, 0, tft.width(), 50, TFT_NAVY);
        tft.setTextColor(TFT_WHITE, TFT_NAVY);
        tft.setTextSize(2);
        tft.setCursor(20, 15);
        tft.println("MiniPixel Interface Ver 0.1");

        // Координаты задают левый верхний угол группы; по умолчанию она центрируется.
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

            //  Сохраняем кординаты кнопок
            active_buttons[i].x = x;
            active_buttons[i].y = buttonsY;
            active_buttons[i].width = buttonWidth;
            active_buttons[i].height = buttonHeight;
            active_buttons[i].id = i;

        }

    }
    uint8_t button_istouched(){
        uint16_t Pos_x, Pos_y;
        if(tft.getTouch(&Pos_x, &Pos_y)){
        for (int i = 0; i<n_buttons; i++){
            if ((active_buttons[i].x < Pos_x && active_buttons[i].width+active_buttons[i].x > Pos_x) && 
                (active_buttons[i].y < Pos_y && active_buttons[i].height+active_buttons[i].y > Pos_y )){
                return active_buttons[i].id;
            }
        }
        return 255; // Есть нажатие, нет попадания по кнопке
        }
        return 254; // Нет нажатия
    };
}; // Конец класса Draw_interface


class Mask_Screen {
    private:
        LGFX& tft; // Внутренняя ссылка на экран

    public:
        // Конструктор без void, со списком инициализации
        Mask_Screen(LGFX& display_ref) : tft(display_ref) {
        }
};