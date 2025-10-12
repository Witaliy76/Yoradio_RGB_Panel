// ===============================================
// КОНФИГУРАЦИЯ ДЛЯ ESP32-4848S040
// ===============================================
// Конфиг для ESP32-4848S040 (ST7701 RGB 480x480 4.0")
// Дисплей: 4.0" IPS LCD 480x480 ST7701S (квадратный)
// Особенности:
// - ST7701S RGB Panel
// - Стандартная подсветка (GPIO38)
// - I2S пины: DOUT=40, BCLK=1, LRC=2
//   by W76W, 4pda.to
#ifndef myoptions_h
#define myoptions_h

/* ===============================================
   DISPLAY (Дисплей)
   =============================================== */
#define DSP_MODEL DSP_ST7701              // Модель дисплея ST7701

// Пины дисплея ESP32-4848S040
// Командная шина ST7701S (SWSPI)
#define ST7701_CS   39       // Chip Select
#define ST7701_SCK  48       // Serial Clock
#define ST7701_SDA  47       // Serial Data
// RGB синхросигналы
#define ST7701_DE     18     // Data Enable
#define ST7701_VSYNC  17     // Vertical Sync
#define ST7701_HSYNC  16     // Horizontal Sync
#define ST7701_PCLK   21     // Pixel Clock
// RGB Data Pins (16-bit interface)
// Red data pins R0..R4 (LSB→MSB)
#define ST7701_R0   11       // R0 (DATA0)
#define ST7701_R1   12       // R1 (DATA1)
#define ST7701_R2   13       // R2 (DATA2)
#define ST7701_R3   14       // R3 (DATA3)
#define ST7701_R4   0        // R4 (DATA4)
// Green data pins G0..G5
#define ST7701_G0   8        // G0 (DATA5)
#define ST7701_G1   20       // G1 (DATA6)
#define ST7701_G2   3        // G2 (DATA7)
#define ST7701_G3   46       // G3 (DATA8)
#define ST7701_G4   9        // G4 (DATA9)
#define ST7701_G5   10       // G5 (DATA10)
// Blue data pins B0..B4 (LSB→MSB)
#define ST7701_B0   4        // B0 (DATA11)
#define ST7701_B1   5        // B1 (DATA12)
#define ST7701_B2   6        // B2 (DATA13)
#define ST7701_B3   7        // B3 (DATA14)
#define ST7701_B4   15       // B4 (DATA15)
// Подсветка (стандартная)
#define ST7701_BL   38       // Backlight pin

/* ===============================================
   I2S DAC (Аудио выход)
   =============================================== */
#define I2S_DOUT     40      // I2S Data Out (GPIO40)
#define I2S_BCLK     1       // I2S Bit Clock (GPIO1)
#define I2S_LRC      2       // I2S Left/Right Clock (GPIO2)

/* ===============================================
   DISABLED FEATURES (Отключенные функции)
   =============================================== */
#define VS1053_CS     255                 // Отключаем VS1053 (не используется)
//#define MUTE_PIN    255                 // Отключаем MUTE (не используется)

/* ===============================================
   BRIGHTNESS CONTROL (Управление яркостью)
   =============================================== */
#define BRIGHTNESS_PIN 255                // Не используем стандартный пин яркости
#define ENABLE_BRIGHTNESS_CONTROL         // Активирует управление яркостью в веб-интерфейсе

/* ===============================================
   GENERAL SETTINGS (Основные настройки)
   =============================================== */
#define PLAYER_FORCE_MONO false           // Моно режим (отключен)
#define L10N_LANGUAGE RU                  // Язык интерфейса (русский)
#define BITRATE_FULL  true                // Полный виджет битрейта

/* ===============================================
   SPECTRUM ANALYZER (Спектроанализатор)
   =============================================== */
// Выбор между VU-метром и спектроанализатором в настройках (usespectrum)
#define SPECTRUM_USE_PSRAM     true       // Использовать PSRAM для FFT буферов
#define SPECTRUM_BANDS         15         // Количество полос спектра
#define SPECTRUM_FFT_SIZE      64         // Размер FFT
#define SPECTRUM_SMOOTHING     0.90f      // Сглаживание (0.0-1.0, больше = плавнее)
#define SPECTRUM_PEAK_HOLD_TIME 300.0f    // Время удержания пиков (мс)
#define SPECTRUM_LOGARITHMIC   false      // Логарифмическая шкала частот
#define SPECTRUM_STEREO        false       // Стерео режим
#define SPECTRUM_GAIN          0.03f      // Общее усиление спектра (1.0 = без усиления)

/* ===============================================
   VU METER & DISPLAY WIDGETS (VU-метр и виджеты)
   =============================================== */
//#define HIDE_VU                         // Скрыть VU-метр (закомментировать для включения)
#define BOOMBOX_STYLE                     // Стиль VU-метра (столбики вместо полос)
#define CPU_LOAD                          // Виджет загрузки процессора
//#define WAKE_PIN              255       // Пин пробуждения (не используется)

/* ===============================================
   SYSTEM & DEBUG (Система и отладка)
   =============================================== */
#define BATTERY_OFF                       // Отключить отображение батареи
//#define PERFMON_DISABLED                // Отключить мониторинг производительности
#define WDT_TIMEOUT 30                    // Таймаут сторожевого таймера (секунды)


/* ===============================================
   SD CARD (SD карта)
   =============================================== */
// Поддержка SD карты включена для ESP32-4848S040
#define USE_SD                              // Включить поддержку SD карты
// Примечание: MISO = D0, MOSI = D1
// SD пины согласно схеме ESP32-4848S040:
//   io42 - TF(D3) - Chip Select
//   io47 - SPICLK_P - MOSI (Master Out Slave In)
//   io48 - SPICLK_N - SCK (Clock)
//   io41 - TF(D1) - MISO (Master In Slave Out)
#define SDC_CS        42                    // Chip Select
#define SD_SCK        48                    // SCK pin
#define SD_MISO       41                    // MISO pin
#define SD_MOSI       47                    // MOSI pin
#define SD_HSPI       true                  // Использовать HSPI для избежания конфликтов
#define SD_DEBUG_ENABLED true               // Отладка SD карты
#define SDSPISPEED    20000000              // Скорость SPI (20 MHz)

/* ===============================================
   TOUCHSCREEN (Тачскрин)
   =============================================== */
// ESP32-4848S040 использует GT911 Capacitive I2C
#define TS_MODEL              TS_MODEL_GT911   // GT911 Capacitive I2C
#define TS_SDA                19               // GPIO19
#define TS_SCL                45               // GPIO45
#define TS_INT                255              // Не используется
#define TS_RST                255              // Не используется

// Калибровка тачскрина (координаты):
#define TS_X_MIN              0                  // Минимум X
#define TS_X_MAX              480                // Максимум X
#define TS_Y_MIN              0                  // Минимум Y
#define TS_Y_MAX              480                // Максимум Y

/* ===============================================
   DEBUG (Отладка)
   =============================================== */
//#define DEBUG_DISPLAY                          // Отладка дисплея

#endif
