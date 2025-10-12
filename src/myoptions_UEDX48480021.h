// ===============================================
// CONFIGURATION FOR UEDX48480021-MD80ET
// ===============================================
// Config for UEDX48480021-MD80ET (ST7701S RGB 480x480 2.1")
// Display: 2.1" IPS LCD 480x480 ST7701S
// Features:
// - ST7701S Type4 preset (BGR mode)
// - RGB→BGR pin swap in Arduino_ESP32RGBPanel
// - Active LOW backlight (GPIO7)
// - I2S pins: DOUT=43, BCLK=44, LRC=4 (GPIO4 requires removing capacitor C9)
//   by W76W, 4pda.to
#ifndef myoptions_h
#define myoptions_h

/* Display */
// New module UEDX48480021-MD80ET (ST7701S RGB 480x480 2.1")
#define DSP_MODEL DSP_UEDX48480021

// Display pins UEDX48480021-MD80ET
// ST7701S command bus (SWSPI)
#define ST7701_CS   18
#define ST7701_SCK  13
#define ST7701_SDA  12
#define ST7701_RST  8
// RGB sync signals
#define ST7701_DE     17
#define ST7701_VSYNC  3
#define ST7701_HSYNC  46
#define ST7701_PCLK   9
// RGB Data Pins (16-bit interface) - CRITICAL SWAP for UEDX48480021
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
// Backlight - CRITICAL: Active LOW for UEDX48480021
#define ST7701_BL   7


/* ===============================================
   ENCODER
   =============================================== */
#define ENC_BTNL              6           // Left rotation
#define ENC_BTNB              0           // Encoder button
#define ENC_BTNR              5           // Right rotation
#define ENC_INTERNALPULLUP    false       // Internal pull-up resistors
#define ENC_HALFQUARD         true        // Half mode (experimental)

/* ===============================================
   DISPLAY OPTIONS
   =============================================== */
#define RSSI_DIGIT            true        // Display RSSI as digits instead of icon

/* ===============================================
   I2S DAC (Audio output)
   =============================================== */
// IMPORTANT: To use GPIO4 (LRC) you must remove capacitor C9 from the board!
#define I2S_DOUT     43      // I2S Data Out (GPIO43, UART TX1 on board)
#define I2S_BCLK     44      // I2S Bit Clock (GPIO44, UART RX1 on board)
#define I2S_LRC      4       // I2S Left/Right Clock (GPIO4, requires C9 removal)


/* ===============================================
   DISABLED FEATURES
   =============================================== */
#define VS1053_CS     255                 // Disable VS1053 (not used)
//#define MUTE_PIN    255                 // Disable MUTE (not used)

/* ===============================================
   BRIGHTNESS CONTROL
   =============================================== */
// IMPORTANT: UEDX48480021 backlight is controlled via Active LOW on GPIO7
#define BRIGHTNESS_PIN 255                // Don't use standard brightness pin
#define ENABLE_BRIGHTNESS_CONTROL         // Enable brightness control in web interface

// Auto-dimming (optional, see main.cpp for implementation)
//#define AUTOBACKLIGHT(x)    *function*    // Autobacklight function. See options.h for example
//#define AUTOBACKLIGHT_MAX     2500
//#define AUTOBACKLIGHT_MIN     12
//#define DOWN_LEVEL           60      // Lowest brightness level (0-255, default 2)
//#define DOWN_INTERVAL        30      // Interval for backlight dimming in seconds (default 60)
//#define GFX_BL      ST7701_BL  // Alias for autobacklight functions

/* ===============================================
   GENERAL SETTINGS
   =============================================== */
#define PLAYER_FORCE_MONO false           // Mono mode (disabled)
#define L10N_LANGUAGE RU                  // Interface language (Russian)
#define BITRATE_FULL  true                // Full bitrate widget

/* ===============================================
   SPECTRUM ANALYZER
   =============================================== */
// Switch between VU-meter and spectrum analyzer in settings (usespectrum)
#define SPECTRUM_USE_PSRAM     true       // Use PSRAM for FFT buffers
#define SPECTRUM_BANDS         10         // Number of spectrum bands (optimized for round display)
#define SPECTRUM_FFT_SIZE      64         // FFT size
#define SPECTRUM_SMOOTHING     0.90f      // Smoothing (0.0-1.0, higher = smoother)
#define SPECTRUM_PEAK_HOLD_TIME 300.0f    // Peak hold time (ms)
#define SPECTRUM_LOGARITHMIC   false      // Logarithmic frequency scale
#define SPECTRUM_STEREO        false      // Stereo mode (disabled - mono)
#define SPECTRUM_GAIN          0.03f      // Overall spectrum gain (1.0 = no gain)

/* ===============================================
   VU METER & DISPLAY WIDGETS
   =============================================== */
//#define HIDE_VU                         // Hide VU-meter (uncomment to enable)
#define BOOMBOX_STYLE                     // VU-meter style (bars instead of strips)
#define CPU_LOAD                          // CPU load widget
//#define WAKE_PIN              255       // Wake pin (not used)

/* ===============================================
   SYSTEM & DEBUG
   =============================================== */
#define BATTERY_OFF                     // Disable battery display
//#define PERFMON_DISABLED                // Disable performance monitoring
#define WDT_TIMEOUT 30                    // Watchdog timeout (seconds)


/* ===============================================
   TOUCHSCREEN
   =============================================== */
// UEDX48480021-MD80ET uses CST826 


// CST826 for UEDX48480021-MD80ET:
#define TS_MODEL              TS_MODEL_CST826  // CST826 Capacitive I2C
#define TS_SDA                16               // GPIO16
#define TS_SCL                15               // GPIO15
#define TS_INT                255              // Not used
#define TS_RST                255              // Not used



// Touchscreen calibration (coordinates):
#define TS_X_MIN              0                  // X minimum
#define TS_X_MAX              480                // X maximum
#define TS_Y_MIN              0                  // Y minimum
#define TS_Y_MAX              480                // Y maximum

/* ===============================================
   DEBUG
   =============================================== */
//#define DEBUG_TOUCH                            // Touchscreen debug
//#define DEBUG_DISPLAY                          // Display debug



#endif
