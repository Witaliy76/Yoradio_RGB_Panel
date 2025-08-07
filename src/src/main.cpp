#include "Arduino.h"
#include "core/options.h"
#include "core/config.h"
#include "core/telnet.h"
#include "core/player.h"
#include "core/display.h"
#include "core/network.h"
#include "core/netserver.h"
#include "core/controls.h"
#include "core/mqtt.h"
#include "core/optionschecker.h"

// Включение Spectrum Analyzer
#if SPECTRUM_ENABLED
#include "displays/tools/spectrum_analyzer.h"
#endif

#if DSP_HSPI || TS_HSPI || VS_HSPI
SPIClass  SPI2(HOOPSENb);
#endif

extern __attribute__((weak)) void yoradio_on_setup();

void setup() {
  Serial.begin(115200);
  if(REAL_LEDBUILTIN!=255) pinMode(REAL_LEDBUILTIN, OUTPUT);
  if (yoradio_on_setup) yoradio_on_setup();
  pm.on_setup();
  config.init();
  display.init();
  
  // Инициализация Spectrum Analyzer
  #if SPECTRUM_ENABLED
  if (!spectrumAnalyzer.init()) {
    Serial.println("[Main] Failed to initialize Spectrum Analyzer!");
  } else {
    Serial.println("[Main] Spectrum Analyzer initialized successfully");
  }
  #endif
  
  player.init();
  network.begin();
  if (network.status != CONNECTED && network.status!=SDREADY) {
    netserver.begin();
    initControls();
    display.putRequest(DSP_START);
    while(!display.ready()) delay(10);
    return;
  }
  if(SDC_CS!=255) {
    display.putRequest(WAITFORSD, 0);
    Serial.print("##[BOOT]#\tSD search\t");
  }
  config.initPlaylistMode();
  netserver.begin();
  telnet.begin();
  initControls();
  display.putRequest(DSP_START);
  while(!display.ready()) delay(10);
  #ifdef MQTT_ROOT_TOPIC
    mqttInit();
  #endif
  if (config.getMode()==PM_SDCARD) player.initHeaders(config.station.url);
  player.lockOutput=false;
  if (config.store.smartstart == 1) player.sendCommand({PR_PLAY, config.lastStation()});
  pm.on_end_setup();
}

void loop() {
  telnet.loop();
  if (network.status == CONNECTED || network.status==SDREADY) {
    player.loop();
    //loopControls();
  }
  loopControls();
  netserver.loop();
}

#include "core/audiohandlers.h"

/**************************************************************************
*   Plugin BacklightDown.
*   Ver.1.0 (Maleksm) for  20.12.2024
***************************************************************************/
#if (defined(AUTOBACKLIGHT)) && (defined(DOWN_LEVEL) && defined(DOWN_INTERVAL))
#include <Ticker.h>

/*    */
#ifdef DOWN_LEVEL
  const uint8_t brightness_down_level = DOWN_LEVEL;
#else
  const uint8_t brightness_down_level = 2;   /* lowest level brightness (from 0 to 255) */
#endif
#ifdef DOWN_INTERVAL
  const uint16_t Out_Interval = DOWN_INTERVAL;
#else
  const uint16_t Out_Interval = 60;         /* interval for BacklightDown in sec (60 sec = 1 min) */
#endif

  Ticker backlightTicker;
  uint8_t current_brightness;

  void backlightDown()       /* function Backlight Down */
  {
  if(network.status!=SOFT_AP)   {
    backlightTicker.detach();
    current_brightness = map(config.store.brightness, 0, 100, 0, 255);
    while(current_brightness > brightness_down_level) {
        current_brightness -= 2;
        if(current_brightness < brightness_down_level) current_brightness = brightness_down_level;
        ledcWrite(0, current_brightness);
        vTaskDelay(30);					                    }
						                    }
  }

  void brightnessOn()          /* function Backlight ON */
  { backlightTicker.detach();
    ledcWrite(0, map(config.store.brightness, 0, 100, 0, 255));
    backlightTicker.attach(Out_Interval, backlightDown);
  }

  void yoradio_on_setup() { brightnessOn(); } 			/* Backlight ON for Setup */
  void player_on_track_change() { brightnessOn(); } 		/* Backlight ON for track change */
  void player_on_start_play() { brightnessOn(); } 		/* Backlight ON for start play */
  void player_on_stop_play() { brightnessOn(); } 		/* Backlight ON for stop play */
  void ctrls_on_loop() { 							/* Backlight ON for reg. operations */
    if(!config.isScreensaver) {
      static uint32_t prevBlPinMillis;
      if((display.mode()!=PLAYER) && (millis()-prevBlPinMillis>1000))
        { prevBlPinMillis=millis();
        brightnessOn(); }
                                    }
    }
#endif  /*  #if defined(AUTOBACKLIGHT) */

// Функция обработки аудио данных для Spectrum Analyzer
#if SPECTRUM_ENABLED
void audio_process_i2s(int16_t* outBuff, uint16_t validSamples, uint8_t bitsPerSample, uint8_t channels, bool *continueI2S) {
    // Обрабатываем аудио данные для Spectrum Analyzer
    spectrumAnalyzer.processAudio(outBuff, validSamples);
    
    // Продолжаем обычную обработку I2S
    *continueI2S = true;
}
#endif
