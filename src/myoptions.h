// Конфиг для UEDX48480021-MD80ET (ST7701S RGB 480x480 2.1")\// ===============================================
// КОНФИГУРАЦИЯ ДЛЯ UEDX48480021-MD80ET
// ===============================================
// Дисплей: 2.1" IPS LCD 480x480 ST7701S
// Особенности:
// - ST7701S Type4 preset (BGR mode)
// - RGB→BGR pin swap в Arduino_ESP32RGBPanel
// - Active LOW backlight (GPIO7)
// - GPIO4 задействован под I2S DOUT
// - I2S пины: DOUT=4, BCLK=43, LRC=44
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


/*  ENCODER  */
#define ENC_BTNL              5           /*  Left rotation */
#define ENC_BTNB              0           /*  Encoder button */
#define ENC_BTNR              6           /*  Right rotation */
#define ENC_INTERNALPULLUP    false          /*  Enable the weak pull up resistors */
#define ENC_HALFQUARD         true          /*  Experiment  with it */
/******************************************/

/*  I2S DAC    */
// Используем пины без конфликтов: DOUT=4 (выведен), BCLK=43, LRC=44 (бывш. UART0)
// Для использования GPIO4 удалить с платы конденсатор C9!!!!
#define I2S_DOUT     43      // GPIO4 (I2S DOUT) uart TX1 на плате
#define I2S_BCLK      44   // GPIO44 (I2S BCLK) uart RX1 на плате
#define I2S_LRC       4   // GPIO43 (I2S LRCK/WS) gpio 4


/* Отключаем функции, которые могут мешать */
#define VS1053_CS     255    // Отключаем VS1053
//#define MUTE_PIN      255    // Отключаем MUTE

#define BRIGHTNESS_PIN 255   // Не используем стандартный пин яркости
#define ENABLE_BRIGHTNESS_CONTROL  // Активирует управление яркостью в веб-интерфейсе
//Изначально не было этого параметра, пришлось его добавить вручную 
//так как управление подсветкой тут отличается 
/* **************************************** *

/* Основные настройки */
#define PLAYER_FORCE_MONO false
#define L10N_LANGUAGE RU
#define BITRATE_FULL  true

// Spectrum Analyzer всегда собран; выбор вида выполняется по usespectrum в настройках
#define SPECTRUM_USE_PSRAM     true    // Использовать PSRAM для FFT буферов
#define SPECTRUM_BANDS         15      // Количество полос спектра (уменьшено до 15)
#define SPECTRUM_FFT_SIZE      64      // Размер FFT (уменьшен для простоты)
#define SPECTRUM_SMOOTHING     0.90f    // Сглаживание (увеличено для более плавных переходов)
#define SPECTRUM_PEAK_HOLD_TIME 300.0f // Время удержания пиков (мс, уменьшено)
#define SPECTRUM_LOGARITHMIC   false   // Логарифмическая шкала частот (отключено)
#define SPECTRUM_STEREO        true   // Стерео режим (отключено - моно)
// #define SPECTRUM_REPLACE_VU    // удалено, выбор вида в рантайме
#define SPECTRUM_GAIN          0.03f    // Общее усиление спектра (1.0 = без усиления, 0.05 = -95%)

// Отключение VU-метра (если используем SA) Закомментировать для включения
//#define HIDE_VU              // выключить тумблер в веб VU !!для выключения включить SPECTRUM_ENABLED и SPECTRUM_REPLACE_VU 

#define BOOMBOX_STYLE    /* Разные варианты "показометра" VUmetr. Столбик, если строку закоментировать. */
//#define WAKE_PIN              255
#define CPU_LOAD        /* Включить виджет загрузки процессора, комментировать для отключения */

/* Отладка - отключаем лишнее */
#define BATTERY_OFF             // Отключаем батарею
//#define PERFMON_DISABLED        // Отключаем мониторинг производительности
#define WDT_TIMEOUT 30          // Увеличиваем таймаут WDT


/*  SDCARD  */
//#define USE_SD                              // Включаем поддержку SD карты
/*  MISO is the same as D0, MOSI is the same as D1 */
/*  SD VSPI PINS. SD SCK must be connected to pin 18
                  SD MISO must be connected to pin 19
                  SD MOSI must be connected to pin 23  */
/*  SD HSPI PINS. SD SCK must be connected to pin 14
                  SD MISO must be connected to pin 12 (+20 KOm на GND)
                  SD MOSI must be connected to pin 13  */
/*  SD PINS согласно схеме 4848S040:
    io42 - TF(D3) - Chip Select
    io47 - SPICLK_P - MOSI (Master Out Slave In)
    io48 - SPICLK_N - SCK (Clock)
    io41 - TF(D1) - MISO (Master In Slave Out)  */
//#define SDC_CS        42                    // Chip Select
//#define SD_SCK        48                    // SCK pin
//#define SD_MISO       41                    // MISO pin  
//#define SD_MOSI       47                    // MOSI pin
//#define SD_HSPI       true                  // Используем HSPI для избежания конфликтов
//#define SD_DEBUG_ENABLED true               // Включаем отладку
//#define SDSPISPEED    20000000              // Скорость SPI (20 MHz)
/* **************************************** */

/*  TOUCHSCREEN  */
/* Touchscreen Configuration для 4848S040 */
//#define TS_MODEL              TS_MODEL_GT911  /* GT911 Capacitive I2C touch screen */
//#define TS_SDA                19              /* Touch screen SDA pin */
//#define TS_SCL                45              /* Touch screen SCL pin */
//#define TS_INT                255             /* Touch screen INT pin (отключен) */
//#define TS_RST                255             /* Touch screen RST pin (отключен) */

// Специальные настройки для квадратного экрана 480x480
//#define SQUARE_SCREEN_OPTIMIZATION true

/*  Resistive SPI touch screen  */
/*  TS VSPI PINS. CLK must be connected to pin 18
                  DIN must be connected to pin 23
                  DO  must be connected to pin 19
                  IRQ - not connected */
//#define TS_CS                 255           /*  Touch screen CS pin  */
/*  TS HSPI PINS. CLK must be connected to pin 14
                  DIN must be connected to pin 13
                  DO  must be connected to pin 12
                  IRQ - not connected */
//#define TS_HSPI               false         /*  Use HSPI for Touch screen  */

/*  Capacitive I2C touch screen GT911  */
#define TS_X_MIN              0               /*  Минимальное значение X координаты  */
#define TS_X_MAX              480             /*  Максимальное значение X координаты  */
#define TS_Y_MIN              0               /*  Минимальное значение Y координаты  */
#define TS_Y_MAX              480             /*  Максимальное значение Y координаты  */
/******************************************/


// Включаем отладку дисплея
//#define DEBUG_DISPLAY



#endif
