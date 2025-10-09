# Yoradio для ESP32 RGB Panel дисплеев

**Проект:** Yoradio для ESP32-S3 с RGB Panel дисплеями  
**Версия:** v0.9.434(m)  
**Статус:** ✅ Активная разработка!

## 🎯 Поддерживаемые дисплеи

Проект поддерживает несколько RGB Panel дисплеев на базе ESP32-S3:

### 📱 ESP32-4848S040 (ST7701S 480x480 4.0" квадратный)
- **Ветка:** `4848S040`
- **Разрешение:** 480x480 пикселей (квадратный)
- **Тачскрин:** GT911 (I2C)
- **Документация:** [README.md](README.md) (этот файл)

### 📱 UEDX48480021-MD80ET (ST7701S 480x480 2.1" круглый)
- **Ветка:** `UEDX48480021`
- **Разрешение:** 480x480 пикселей (круглый, эффективная область ~400x400)
- **Тачскрин:** CST826 (I2C)
- **Документация:** [README_UEDX48480021.md](README_UEDX48480021.md)

## 🎯 Описание проекта

Порт проекта [Yoradio](https://github.com/e2002/yoradio) для ESP32-S3 модулей с **RGB Panel дисплеями ST7701S**:

### Общие характеристики:
- **Разрешение:** 480x480 пикселей
- **Интерфейс:** RGB Panel (параллельный, 16 линий данных)
- **Процессор:** ESP32-S3-WROOM-1-N16R8 (240MHz, dual-core)
- **Память:** 520KB RAM + 16MB PSRAM + 8MB Flash
- **Аудио:** I2S DAC
- **Библиотеки:** I2S 3.3.2, Arduino-ESP32 3.x

### Поддерживаемые модули:
1. **ESP32-4848S040** - квадратный дисплей 4.0", тачскрин GT911
2. **UEDX48480021-MD80ET** - круглый дисплей 2.1", тачскрин CST826

![photo_2025-08-15_00-06-22](https://github.com/user-attachments/assets/bb047124-7e1a-4d0a-8f70-b78dccf4fd36)
*Spectrum Analyzer Mode*

![photo_2025-09-22_20-27-50](https://github.com/user-attachments/assets/b5d05ff3-b0bf-4b8a-866a-7c32e659e3fa)
*VU meter mode* 

## 📝 Лог изменений

**09.10.2025** - Добавлена поддержка круглого дисплея UEDX48480021-MD80ET:
- ✅ Добавлен новый дисплей UEDX48480021 (ST7701S 480x480 2.1" круглый)
- ✅ Полная адаптация всех виджетов под круглую форму дисплея
- ✅ Создан отдельный README_UEDX48480021.md с документацией
- ✅ Исправлены баги с очисткой часов и heapbar для всех дисплеев
- ✅ Структурированы конфигурационные файлы myoptions.h
- ✅ Проект теперь поддерживает несколько RGB Panel дисплеев

**29.09.2025** 4848S040- Рефакторинг конфигурации спектр-анализатора и фикс виджета битрейта:
- ✅ Перенесена конфигурация `SpectrumWidgetConfig` из отдельного файла `spectrum_st7701.h` в основной конфигурационный файл `displayST7701conf.h`
- ✅ Упрощена архитектура проекта - все настройки виджетов дисплея теперь в одном месте
- ✅ Удален файл `spectrum_st7701.h` как больше не нужный
- ✅ Обновлен `display.cpp` для использования новой структуры конфигурации
- ✅ Применен фикс переполнения виджета битрейта в `widgets.cpp`
- ✅ Добавлена проверка границ для отображения битрейта и формата файла
- ✅ Автоматическое переключение на целые числа при переполнении виджета
- ✅ Протестирована компиляция - все работает корректно
- Оптимизация autoflush 
- ✅ Отключен autoflush в Arduino_RGB_Display для улучшения производительности

**12.09.2025** 4848S040- SA ↔ VU переключатель в вебе и стабильность отрисовки:
- ✅ Добавлен тумблер `spectrum analyzer` на странице Settings → Screen (рядом с `numbered playlist`)
- ✅ VU: инверсия уровней перенесена после сглаживания (корректно работает `fadespeed`), дельта‑отрисовка без фликера,  аудиобиблиотека инвертирует вывод для VU.(?)



**09.09.2025** 4848S040- Добавил файлы bin для заливки в устройство:
- ✅ bootloader.bin
- ✅ firmware.bin
- ✅ partitions.bin
- ✅ Добавлены в каталог build_bin/

**03.09.2025** 4848S040- Исправлена проблема с отображением текста заглавными буквами:
- ✅ Исправлена функция gfxDrawText для поддержки параметра uppercase
- ✅ Обновлены все виджеты (TextWidget, ScrollWidget, NumWidget, BitrateWidget)
- ✅ Исправлено отображение в дисплее ST7701 (дата, время, плейлист)
- ✅ Обеспечено единообразное отображение всех элементов плейлиста
- ✅ Текст теперь отображается согласно настройкам конфигурации

**03.09.2025** 4848S040 - Исправлены оси тачскрина для выбора станций:
- ✅ Свайп вверх → следующая станция
- ✅ Свайп вниз → предыдущая станция
- ✅ Оптимизирована логика обработки свайпов

**02.09.2025** 4848S040 - Добавлена интеграция SD карты:
- ✅ Настроена конфигурация SPI для SD карты (FSPI/SPI2_HOST)
- ✅ Исправлен deadlock в mutex для функции `_checkNoMedia()`
- ✅ SD карта успешно инициализируется и индексируется
- ✅ Плейлист создается и корректно читается
- ✅ Список композиций отображается и листается
- ❌ **ПРОБЛЕМА:** Воспроизведение не работает - система зависает в `config.loadStation()`

**См. [ISSUE: SD Card Playback Problem](#-sd-card-playback-issue)**

> **⚠️ ВАЖНО:** Для стабильной работы интернет-радио на больших битрейтах необходимо заменить стандартные библиотеки IDF на перекомпилированные версии. См. раздел [Установка библиотек](#-установка-библиотек).




## 🚀 Быстрый старт

### Выбор дисплея:
Скопируйте нужный конфигурационный файл в `src/myoptions.h`:
- Для **ESP32-4848S040**: `cp src/myoptions_4848S040.h src/myoptions.h`
- Для **UEDX48480021**: `cp src/myoptions_UEDX48480021.h src/myoptions.h`

## 🚀 Общие особенности проекта

- ✅ **ST7701S RGB Panel 480x480** - высокое разрешение для обоих дисплеев
- ✅ **Поддержка нескольких дисплеев** - квадратный 4.0" и круглый 2.1"
- ✅ **Спектральный анализатор** - переключение VU-метр ↔ SA в реальном времени
- ✅ **Русская локализация** - полная поддержка
- ✅ **WiFi интернет-радио** - основной функционал
- ✅ **Оптимизация для ESP32-S3** - максимальная производительность
- ✅ **Тачскрин** - GT911 (4848S040) или CST826 (UEDX48480021)
- ✅ **Энкодер** - альтернативное управление

## 🔧 Сборка

Для русской локализации заменяем файл fonts/glcdfont.h по адресу: pio\libdeps\yoradio-esp32s3\GFX Library for Arduino\src\font


## 📱 Конфигурация проекта

### Основные файлы:
- **`src/myoptions.h`** - активный конфигурационный файл (копируется из myoptions_XXX.h)
- **`src/myoptions_4848S040.h`** - конфигурация для ESP32-4848S040
- **`src/myoptions_UEDX48480021.h`** - конфигурация для UEDX48480021-MD80ET
- **`src/mytheme.h`** - цветовая схема интерфейса

### Файлы дисплеев:
- **`src/src/displays/displayST7701.cpp`** - драйвер для ESP32-4848S040
- **`src/src/displays/displayUEDX48480021.cpp`** - драйвер для UEDX48480021
- **`src/src/displays/conf/displayST7701conf.h`** - конфигурация виджетов 4848S040
- **`src/src/displays/conf/displayUEDX48480021conf.h`** - конфигурация виджетов UEDX48480021

## 🔧 Установка библиотек

Для решения проблемы заикания интернет-радио на больших битрейтах необходимо заменить стандартные библиотеки IDF на перекомпилированные версии:

### 📁 Замена библиотек

1. **Найдите папку PlatformIO:**
   ```
   <user>/.platformio/packages/framework-arduinoespressif32-libs/esp32s3/lib/
   ```

2. **Замените файлы:**
   - `libesp_netif.a` → `libesp_netif.a` (из архива)
   - `liblwip.a` → `liblwip.a` (из архива)

3. **Поддерживаемые версии IDF:**
   - ✅ ESP-IDF 5.4
   - ✅ ESP-IDF 5.5

### 📦 Архив с библиотеками

Перекомпилированные библиотеки находятся в папке `library!/esp32s3_5.4/`:
- `libesp_netif.a` (496KB)
- `liblwip.a` (3.8MB)

> **Примечание:** После замены библиотек перезапустите PlatformIO и перекомпилируйте проект.

## 🎮 Управление

- **Одиночное касание** - старт/стоп
- **Свайп влево/вправо** - изменение громкости (оптимизировано)
- **Свайп вверх/вниз** - выбор станции (оптимизировано)
- **Двойное касание** - смена режима (RADIO ↔ SD)
- **Энкодер** - навигация и управление громкостью

## 📊 Сравнение дисплеев

| Параметр | ESP32-4848S040 | UEDX48480021-MD80ET |
|----------|----------------|---------------------|
| **Размер** | 4.0" квадратный | 2.1" круглый |
| **Разрешение** | 480x480 | 480x480 (~400x400 эффективно) |
| **Тачскрин** | GT911 (SDA:19, SCL:45) | CST826 (SDA:16, SCL:15) |
| **I2S пины** | DOUT:40, BCLK:1, LRC:2 | DOUT:43, BCLK:44, LRC:4 |
| **Подсветка** | GPIO38 (стандартная) | GPIO7 (Active LOW) |
| **Энкодер** | Стандартный | GPIO 0, 5, 6 |
| **Особенности** | SD карта | Требует удаления C9 |
| **Формат** | RGB565 (16-bit) | RGB565 (16-bit, BGR swap) |

## 🔍 Отладка

```cpp
// Включить отладку дисплея
#define DEBUG_DISPLAY

// Включить отладку тачскрина (опционально)
#define DEBUG_TOUCH
```

4848S040: Порядок подключения DAC к разъему:
![S3c5b9bd4f2a54006af227f2b4acd0fc1b](https://github.com/user-attachments/assets/c49b36b6-3b10-4db5-ad03-bf7ecc4aca3d)

![photo_2025-09-09_19-40-49](https://github.com/user-attachments/assets/e45c9c79-ed04-4fc9-8e6e-d9d97bda08d4)

в модуле есть встренный моно DAC. По умолчанию отключен - пины переключены на задний разъем. 
Для переключения модуля на встроенный DAC - запаиваем перемычки R21, R22, R23. 
Если, при этом, задним разьемом пользоваться не будете - то резисторы R26, R27, R25 можно не убирать.. иначе их нужно выпаять


![photo_2025-09-09_19-57-01](https://github.com/user-attachments/assets/2abafab0-3ece-4569-8181-4b2ded7a6a62)



## 🐛 Известные проблемы 4848S040

- ❌ **SD карта: воспроизведение не работает** (см. раздел ниже)


## 🚨 SD Card Playback Issue

### Problem Description
SD card integration works perfectly for indexing and playlist reading during boot/initialization, but **hangs during audio playback** when user presses Play button.

### Specific Failure Point
The system hangs in `config.loadStation()` function when attempting to open SD card files:
```cpp
File playlist = SDPLFS()->open(REAL_PLAYL, "r");  // ← HANGS HERE
File index = SDPLFS()->open(REAL_INDEX, "r");     // ← NEVER REACHED
```

### Working Scenarios
- ✅ SD card initialization and mounting
- ✅ SD card indexing (creates playlistsd.csv and indexsd.dat)
- ✅ Playlist reading during boot sequence
- ✅ All cardPresent() checks pass
- ✅ File existence checks pass

### Failing Scenario
- ❌ Audio playback when user presses Play button
- ❌ System hangs on SDPLFS()->open() calls in loadStation()

### Technical Details
- **Board:** ESP32-S3 N16R8
- **Display:** ST7701 RGB 480x480 (uses VSPI)
- **SD Card:** FSPI (SPI2_HOST) with custom pins: CS=42, SCK=48, MISO=41, MOSI=47
- **Audio:** I2S (no SPI conflict)
- **FreeRTOS:** mutex system implemented for thread safety

### Attempted Solutions
1. Changed SD card SPI from FSPI to HSPI - no effect
2. Added mutex timeouts and forced release - no effect
3. Simplified cardPresent() check (removed readRAW) - no effect
4. Suspended display task during SD access - no effect
5. Removed all SD state checks - still hangs on file open

### Root Cause Hypothesis
The issue appears to be in the SD card file system layer (SDPLFS()->open()) rather than SPI communication or mutex management. The same SD card operations work fine during boot but fail during runtime playback context.

### Developer Request
If you can identify why SDPLFS()->open() works during boot but hangs during playback, please help resolve this issue. The SD card hardware and basic file operations are confirmed working.

## 📝 Лицензия

MIT License - см. файл [LICENSE](LICENSE)

---



