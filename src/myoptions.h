// Тестовый конфиг для ST7701 (ESP32-4848S040)
#ifndef myoptions_h
#define myoptions_h

/* Display */
// Новый модуль ESP32-4848S040 (ST7701 RGB 480x480)
#define DSP_MODEL DSP_ST7701

// Пины дисплея (перенесены из тестового скетча)
// Командная шина ST7701 (SWSPI)
#define ST7701_CS   39
#define ST7701_SCK  48
#define ST7701_SDA  47
// RGB синхросигналы
#define ST7701_DE     18
#define ST7701_VSYNC  17
#define ST7701_HSYNC  16
#define ST7701_PCLK   21
// R0..R4
#define ST7701_R0   4
#define ST7701_R1   5
#define ST7701_R2   6
#define ST7701_R3   7
#define ST7701_R4   15
// G0..G5
#define ST7701_G0   8
#define ST7701_G1   20
#define ST7701_G2   3
#define ST7701_G3   46
#define ST7701_G4   9
#define ST7701_G5   10
// B0..B4
#define ST7701_B0   11
#define ST7701_B1   12
#define ST7701_B2   13
#define ST7701_B3   14
#define ST7701_B4   0
// Подсветка
#define ST7701_BL   38

/*  I2S DAC    */
#define I2S_DOUT      41
#define I2S_BCLK      42
#define I2S_LRC       2

/* Отключаем функции, которые могут мешать */
#define VS1053_CS     255    // Отключаем VS1053
#define SDC_CS        255    // Отключаем SD
#define MUTE_PIN      255    // Отключаем MUTE
#define BRIGHTNESS_PIN 255   // Не используем стандартный пин яркости

/* Основные настройки */
#define PLAYER_FORCE_MONO false
#define L10N_LANGUAGE RU
#define BITRATE_FULL  true

/* Spectrum Analyzer */
#define SPECTRUM_ENABLED false  // Отключаем для первого теста
#define SPECTRUM_GAIN 1.0       // Если включен спектр

/* Отладка - отключаем лишнее */
#define BATTERY_OFF             // Отключаем батарею
#define PERFMON_DISABLED        // Отключаем мониторинг производительности
#define WDT_TIMEOUT 30          // Увеличиваем таймаут WDT

// Отключаем тачскрин для первого теста
//#define TS_MODEL TS_MODEL_UNDEFINED

// Включаем отладку дисплея
#define DEBUG_DISPLAY

#endif
