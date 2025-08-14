// Тестовый конфиг для ST7701 (ESP32-4848S040)
#ifndef myoptions_h
#define myoptions_h

/* Display */
// Новый модуль ESP32-4848S040 (ST7701 RGB 480x480)
#define DSP_MODEL DSP_ST7701

// Пины дисплея 
// Командная шина ST7701 (SWSPI)
#define ST7701_CS   39
#define ST7701_SCK  48
#define ST7701_SDA  47
// RGB синхросигналы
#define ST7701_DE     18
#define ST7701_VSYNC  17
#define ST7701_HSYNC  16
#define ST7701_PCLK   21
// R0..R4 (LSB→MSB) — согласно вендору
#define ST7701_R0   11
#define ST7701_R1   12
#define ST7701_R2   13
#define ST7701_R3   14
#define ST7701_R4   0
// G0..G5
#define ST7701_G0   8
#define ST7701_G1   20
#define ST7701_G2   3
#define ST7701_G3   46
#define ST7701_G4   9
#define ST7701_G5   10
// B0..B4 (LSB→MSB) — согласно вендору
#define ST7701_B0   4
#define ST7701_B1   5
#define ST7701_B2   6
#define ST7701_B3   7
#define ST7701_B4   15
// Подсветка
#define ST7701_BL   38

/*  I2S DAC    */
#define I2S_DOUT      40
#define I2S_BCLK      1
#define I2S_LRC       2

/* Отключаем функции, которые могут мешать */
#define VS1053_CS     255    // Отключаем VS1053
#define SDC_CS        255    // Отключаем SD
#define MUTE_PIN      255    // Отключаем MUTE
#define BRIGHTNESS_PIN 255   // Не используем стандартный пин яркости
#define ENABLE_BRIGHTNESS_CONTROL  // Активирует управление яркостью в веб-интерфейсе
//Изначально не было этого параметра, пришлось его добавить вручную 
//так как управление подсветкой тут отличается 
/* **************************************** *

/* Основные настройки */
#define PLAYER_FORCE_MONO true
#define L10N_LANGUAGE RU
#define BITRATE_FULL  true

// Включение Spectrum Analyzer
#define SPECTRUM_ENABLED       true    // !!Включить Spectrum Analyzer
#define SPECTRUM_USE_PSRAM     true    // Использовать PSRAM для FFT буферов
#define SPECTRUM_BANDS         15      // Количество полос спектра (уменьшено до 15)
#define SPECTRUM_FFT_SIZE      64      // Размер FFT (уменьшен для простоты)
#define SPECTRUM_SMOOTHING     0.90f    // Сглаживание (увеличено для более плавных переходов)
#define SPECTRUM_PEAK_HOLD_TIME 300.0f // Время удержания пиков (мс, уменьшено)
#define SPECTRUM_LOGARITHMIC   false   // Логарифмическая шкала частот (отключено)
#define SPECTRUM_STEREO        true   // Стерео режим (отключено - моно)
#define SPECTRUM_REPLACE_VU    true   // !!Заменить VU-метр на SA
#define SPECTRUM_GAIN          0.03f    // Общее усиление спектра (1.0 = без усиления, 0.05 = -95%)

// Отключение VU-метра (если используем SA) Закомментировать для включения
//#define HIDE_VU              // выключить тумблер в веб VU !!для выключения включить SPECTRUM_ENABLED и SPECTRUM_REPLACE_VU 
#define BOOMBOX_STYLE    /* Разные варианты "показометра" VUmetr. Столбик, если строку закоментировать. */
//#define WAKE_PIN              255
//#define CPU_LOAD        /* Включить виджет загрузки процессора, комментировать для отключения */

/* Отладка - отключаем лишнее */
#define BATTERY_OFF             // Отключаем батарею
//#define PERFMON_DISABLED        // Отключаем мониторинг производительности
#define WDT_TIMEOUT 30          // Увеличиваем таймаут WDT

// Отключаем тачскрин для первого теста
//#define TS_MODEL TS_MODEL_UNDEFINED

// Включаем отладку дисплея
//#define DEBUG_DISPLAY

#endif
