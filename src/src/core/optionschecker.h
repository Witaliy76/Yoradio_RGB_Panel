#ifndef optionschecker_h
#define optionschecker_h

#if REAL_LEDBUILTIN==TFT_RST
#  error LED_BUILTIN IS THE SAME AS TFT_RST. Check it in myoptions.h
#endif

#if REAL_LEDBUILTIN==VS1053_RST
#  error LED_BUILTIN IS THE SAME AS VS1053_RST. Check it in myoptions.h
#endif

/* Builtin LED vs shared GPIO.
   On ESP32-S3 variants with an RGB LED (Arduino Core 3.x, variant esp32s3), LED_BUILTIN is
   a virtual pin `static const uint8_t LED_BUILTIN = SOC_GPIO_PIN_COUNT + PIN_RGB_LED`:
   digitalWrite() drives it through RMT on the physical GPIO. It is a C++ constant, not a
   numeric macro, so it cannot be checked with #if (it silently evaluates to 0 there).
   Resolve the alias at compile time and refuse configurations where the LED shares a pin
   with SD or the ST7701 command bus. 255 means "not connected". */
#include "soc/soc_caps.h"
namespace yo_optcheck {
  constexpr int ledPhys(int led) {
    return (led == 255) ? 255 : (led >= SOC_GPIO_PIN_COUNT ? led - SOC_GPIO_PIN_COUNT : led);
  }
  constexpr int kLed = ledPhys(REAL_LEDBUILTIN);
  constexpr bool clash(int pin) { return kLed != 255 && pin != 255 && kLed == pin; }
  static_assert(!clash(SDC_CS), "Builtin LED (physical GPIO) == SDC_CS. Set USE_BUILTIN_LED false in myoptions.h");
#ifdef SD_SPIPINS
  constexpr int kSdPins[] = { SD_SPIPINS };
  static_assert(!clash(kSdPins[0]) && !clash(kSdPins[1]) && !clash(kSdPins[2]),
                "Builtin LED (physical GPIO) conflicts with SD_SPIPINS. Set USE_BUILTIN_LED false in myoptions.h");
#endif
#ifdef ST7701_SCK
  static_assert(!clash(ST7701_SCK), "Builtin LED (physical GPIO) == ST7701_SCK. Set USE_BUILTIN_LED false in myoptions.h");
#endif
#ifdef ST7701_SDA
  static_assert(!clash(ST7701_SDA), "Builtin LED (physical GPIO) == ST7701_SDA. Set USE_BUILTIN_LED false in myoptions.h");
#endif
#ifdef ST7701_CS
  static_assert(!clash(ST7701_CS), "Builtin LED (physical GPIO) == ST7701_CS. Set USE_BUILTIN_LED false in myoptions.h");
#endif
}

#if (I2S_DOUT!=255) && (VS1053_CS!=255)
#  error YOU MUST CHOOSE BETWEEN I2S DAC AND VS1053 BY DISABLING THE SECOND MODULE IN THE myoptions.h
#endif

#if !(defined(ARDUINO_ESP32_DEV) || defined(ARDUINO_ESP32S3_DEV) || defined(ARDUINO_ESP32C3_DEV))
#  error ONLY MODULES "ESP32 Dev Module", "ESP32 Wrover Module" AND "ESP32 S3 Dev Module" ARE SUPPORTED. PLEASE SELECT ONE OF THEM IN THE MENU >> TOOLS >> BOARD
#endif

#endif


