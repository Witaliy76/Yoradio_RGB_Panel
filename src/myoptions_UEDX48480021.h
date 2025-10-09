// ===============================================
// КОНФИГУРАЦИЯ ДЛЯ UEDX48480021-MD80ET
// ===============================================
// Конфиг для UEDX48480021-MD80ET (ST7701S RGB 480x480 2.1")
// Дисплей: 2.1" IPS LCD 480x480 ST7701S
// Особенности:
// - ST7701S Type4 preset (BGR mode)
// - RGB→BGR pin swap в Arduino_ESP32RGBPanel
// - Active LOW backlight (GPIO7)
// - I2S пины: DOUT=43, BCLK=44, LRC=4 (GPIO4 требует удаления конденсатора C9)
//   by W76W, 4pda.to
#ifndef myoptions_h
#define myoptions_h

/* Display */
// Новый модуль UEDX48480021-MD80ET (ST7701S RGB 480x480 2.1")
#define DSP_MODEL DSP_UEDX48480021

// Пины дисплея UEDX48480021-MD80ET
// Командная шина ST7701S (SWSPI)
#define ST7701_CS   18
#define ST7701_SCK  13
#define ST7701_SDA  12
#define ST7701_RST  8
// RGB синхросигналы
#define ST7701_DE     17
#define ST7701_VSYNC  3
#define ST7701_HSYNC  46
#define ST7701_PCLK   9
// RGB Data Pins (16-bit interface) - CRITICAL SWAP для UEDX48480021
// Red data pins R0..R4 - SWAPPED with B pins
#define ST7701_R0   10   // B0 (DATA0)  → R0 (SWAPPED)
#define ST7701_R1   11   // B1 (DATA1)  → R1 (SWAPPED)
#define ST7701_R2   12   // B2 (DATA2)  → R2 (SWAPPED)
#define ST7701_R3   13   // B3 (DATA3)  → R3 (SWAPPED)
#define ST7701_R4   14   // B4 (DATA4)  → R4 (SWAPPED)
// Green data pins G0..G5
#define ST7701_G0   21   // G0 (DATA5)
#define ST7701_G1   47   // G1 (DATA6)
#define ST7701_G2   48   // G2 (DATA7)
#define ST7701_G3   45   // G3 (DATA8)
#define ST7701_G4   38   // G4 (DATA9)
#define ST7701_G5   39   // G5 (DATA10)
// Blue data pins B0..B4 - SWAPPED with R pins
#define ST7701_B0   40   // R0 (DATA11) → B0 (SWAPPED)
#define ST7701_B1   41   // R1 (DATA12) → B1 (SWAPPED)
#define ST7701_B2   42   // R2 (DATA13) → B2 (SWAPPED)
#define ST7701_B3   2    // R3 (DATA14) → B3 (SWAPPED)
#define ST7701_B4   1    // R4 (DATA15) → B4 (SWAPPED)
// Подсветка - CRITICAL: Active LOW для UEDX48480021
#define ST7701_BL   7


/* ===============================================
   ENCODER (Энкодер)
   =============================================== */
#define ENC_BTNL              6           // Левое вращение (Left rotation)
#define ENC_BTNB              0           // Кнопка энкодера (Encoder button)
#define ENC_BTNR              5           // Правое вращение (Right rotation)
#define ENC_INTERNALPULLUP    false       // Внутренние подтягивающие резисторы
#define ENC_HALFQUARD         true        // Половинный режим (экспериментально)

/* ===============================================
   DISPLAY OPTIONS (Опции дисплея)
   =============================================== */
#define RSSI_DIGIT            true        // Отображать RSSI цифрами вместо иконки

/* ===============================================
   I2S DAC (Аудио выход)
   =============================================== */
// ВАЖНО: Для использования GPIO4 (LRC) необходимо удалить конденсатор C9 с платы!
#define I2S_DOUT     43      // I2S Data Out (GPIO43, UART TX1 на плате)
#define I2S_BCLK     44      // I2S Bit Clock (GPIO44, UART RX1 на плате)
#define I2S_LRC      4       // I2S Left/Right Clock (GPIO4, требует удаления C9)


/* ===============================================
   DISABLED FEATURES (Отключенные функции)
   =============================================== */
#define VS1053_CS     255                 // Отключаем VS1053 (не используется)
//#define MUTE_PIN    255                 // Отключаем MUTE (не используется)

/* ===============================================
   BRIGHTNESS CONTROL (Управление яркостью)
   =============================================== */
// ВАЖНО: Подсветка UEDX48480021 управляется через Active LOW на GPIO7
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
#define SPECTRUM_BANDS         10         // Количество полос спектра (оптимизировано для круглого дисплея)
#define SPECTRUM_FFT_SIZE      64         // Размер FFT
#define SPECTRUM_SMOOTHING     0.90f      // Сглаживание (0.0-1.0, больше = плавнее)
#define SPECTRUM_PEAK_HOLD_TIME 300.0f    // Время удержания пиков (мс)
#define SPECTRUM_LOGARITHMIC   false      // Логарифмическая шкала частот
#define SPECTRUM_STEREO        false      // Стерео режим (отключено - моно)
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
#define BATTERY_OFF                     // Отключить отображение батареи
//#define PERFMON_DISABLED                // Отключить мониторинг производительности
#define WDT_TIMEOUT 30                    // Таймаут сторожевого таймера (секунды)


/* ===============================================
   TOUCHSCREEN (Тачскрин)
   =============================================== */
// UEDX48480021-MD80ET использует CST826 


// CST826 для UEDX48480021-MD80ET:
#define TS_MODEL              TS_MODEL_CST826  // CST826 Capacitive I2C
#define TS_SDA                16               // GPIO16
#define TS_SCL                15               // GPIO15
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
//#define DEBUG_TOUCH                            // Отладка тачскрина
//#define DEBUG_DISPLAY                          // Отладка дисплея



#endif
