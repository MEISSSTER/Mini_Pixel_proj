#include <Arduino.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include "pin_config.h"
#include "screen_classes.hpp"

// 1. Создаем дисплеи физически (передаем параметры конструктору LGFX)
// tft_main (Ядро 1) - шина SPI3, тачскрин есть, shared = false
LGFX tft_main(SPI3_HOST, PIN_TFT0_SCLK, PIN_TFT0_MOSI, PIN_TOUCH_MISO, PIN_TFT0_CS, PIN_TFT0_DC, PIN_TFT0_RST, PIN_TOUCH_CS, false);

// tft_mask (Ядро 0) - шина SPI2, тачскрина нет (MISO=-1, TOUCH_CS=-1), shared = true
LGFX tft_mask(SPI2_HOST, PIN_TFT1_SCLK, PIN_TFT1_MOSI, -1, PIN_TFT1_CS, PIN_TFT1_DC, PIN_TFT1_RST, -1, true);

// 2. Создаем обертки логики, передавая в них ссылки на готовые экраны
Draw_interface screen(tft_main);
Mask_Screen mask(tft_mask);

// String choose_model(String ModelName){
//     File root = SD.open("/");
//     String model_path = "";
//     while(root){
//         if((root.filename() == ModelName) && root.isDirectory()){
//             snprint(model_path, "/")
//             return 
//         }
//     }
// }

// Функция поиска папки с PNG файлами
String find_name() {
    File root = SD.open("/");
    if (!root) {
        Serial.println("Ошибка открытия корня SD-карты");
        return "";
    }

    File entry = root.openNextFile();
    while (entry) {
        String folderName = entry.name();

        // Проверяем, что это папка (исключаем системные)
        if (entry.isDirectory() && folderName != "System Volume Information" && !folderName.startsWith(".")) {
            Serial.print("Обнаружена папка: ");
            Serial.println(folderName);

            bool hasPng = false; // Объявляем флаг
            File child = entry.openNextFile(); // Проваливаемся внутрь найденной папки
            
            while (child) {
                String childName = child.name();
                
                // Если это файл и он заканчивается на .png
                if (!child.isDirectory() && childName.endsWith(".png")) {
                    hasPng = true;
                    child.close(); 
                    break; // Нашли PNG, выходим из внутреннего цикла
                }
                child.close(); 
                child = entry.openNextFile(); // Берем следующий файл в папке
            }

            // Если PNG найден, возвращаем имя папки
            if (hasPng) {
                Serial.println("Содержит .png, принимаем за директорию модели");
                
                // Убираем слэш в начале, если он есть
                if (folderName.startsWith("/")) {
                    folderName = folderName.substring(1);
                }
                
                entry.close();
                root.close();
                return folderName;
            }
        }
        entry.close();
        entry = root.openNextFile(); // Берем следующую папку в корне
    }
    
    root.close();
    Serial.println("Папки с PNG не найдены!");
    return "";
}

// Фоновая задача для Ядра 0 (Строгая сигнатура FreeRTOS)
void Core0TaskManager(void *pvParameters) { 
    // Забираем указатель на наш объект маски
    Mask_Screen* mask_manager = static_cast<Mask_Screen*>(pvParameters);
    
    uint32_t frame_num = 0;

    for(;;) {
        // Здесь можно обращаться к mask_manager->...

        vTaskDelay(pdMS_TO_TICKS(100)); // Отдаем процессорное время
    }
}
void setup() {
    Serial.begin(115200);
    Serial.println("Serial start   |SUCCESS|");
    // 1. Запуск UI (Экран монополист, ничего не ждет)
    tft_main.init();
    tft_main.setRotation(1);
    
    // 2. Запуск шины SPI2 и SD-карты
    SPI.begin(PIN_SD_SCLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    if (!SD.begin(PIN_SD_CS, SPI)) {
        Serial.println("Ошибка SD-карты!   |ERROR|");
    } else {
        Serial.println("SD-карта подключена   |SUCCESS|");
    }

    String modelname = find_name();
    
    

    // 3. Запуск Маски (Она подхватит уже запущенную шину SPI2)
    tft_mask.init();
    
    // 4. Отрисовка
    screen.drawDashboardLayout(); 

    TaskHandle_t maskTaskHandle = nullptr;
    xTaskCreatePinnedToCore(
        Core0TaskManager, "Core1TaskManager", 4096, nullptr,
        1, &maskTaskHandle, 0);
}

void loop() {
    // Проверка кнопок (для теста)
    uint8_t btn = screen.button_istouched();
    if (btn < 254) {
        Serial.print("Нажата кнопка ID: ");
        Serial.println(btn);
        vTaskDelay(200); // Простой антидребезг
    }
}