# **Mini_Pixel**
# Текущие задачи: https://app.striveapp.ru/join/86dc772f-b0de-4d36-9c0d-0f7b8b92f3bd

## Написание кода
Код для МК ESP32-S3, код можно писать в Visual Studio Code с дополнением PlatformIO, на данный момент тестируется и будет проверятся.
Не забываем загрузжать всё на GitHub, делать это через VS Code можно из коробки, главное иметь Git на компьютере, и настроить чуток (https://habr.com/ru/articles/541258/).

## Тестирование логической схемы
Будет проводится в Proteus

|№|Элементы|Ссылка на документацию| Коментарии |
|---|---|---|---|
|1.| ESP32-S3 | [Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf) | Основной контроллер. Логика 3.3В. |
|2.| DC CONVERTER LM2596 | [Datasheet](https://www.ti.com/lit/ds/symlink/lm2596.pdf) | Понижающий модуль. Настроить на 5В перед подключением. |
|3.| SONGLE SRD-05VDC-SL-C | [Datasheet](https://html.alldatasheet.com/html-pdf/99638/SONGLE/SRD-05VDC-SL-C/238/1/SRD-05VDC-SL-C.html) | Реле 5В. Управлять через транзистор + защитный диод. |
|4.| ili9488 x2 | [Datasheet](https://www.displayfuture.com/Display/Datasheet/Controller/ILI9488.pdf) | В Proteus можно использовать аналог (ILI9341). Нужны разные CS. |
|5.| Шаговый двигатель nema17 17mm | [Specs](https://www.reprap.org/wiki/NEMA_17_Stepper_motor) | Требуется внешний драйвер (A4988 или TMC2209). |

### Ссылки на скачку
|№|Программа|Ссылка / Инструкция|
|-|-|-|
|1.| Visual Studio Code | https://code.visualstudio.com/download |
|2.| PlatformIO | https://docs.platformio.org/en/latest/integration/ide/vscode.html | 
|3.| Git | https://git-scm.com/install/windows / https://habr.com/ru/articles/541258/|
|4.| Proteus | Пираты впрерёд, пока нет ссылки на нормальный файл |



