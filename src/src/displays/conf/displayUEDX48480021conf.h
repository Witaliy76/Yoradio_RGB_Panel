/*************************************************************************************
    UEDX48480021 480x480 displays configuration file.
    Copy this file to yoRadio/src/displays/conf/displayUEDX48480021conf_custom.h
    and modify it
    More info on https://github.com/e2002/yoradio/wiki/Widgets#widgets-description
*************************************************************************************/

#ifndef displayUEDX48480021conf_h
#define displayUEDX48480021conf_h

#include "../widgets/widgets.h"
#include "../tools/spectrum_widget.h"

// Hide VU-meter (if needed)
// #define HIDE_VU

// Hide second title (title2) - saves space on round display
#define HIDE_TITLE2

#define DSP_WIDTH       480
#define DSP_HEIGHT      480
#define TFT_FRAMEWDT    8     // Frame width (border offset)
#define MAX_WIDTH       DSP_WIDTH-TFT_FRAMEWDT

#if BITRATE_FULL
  #define TITLE_FIX 44
#else
  #define TITLE_FIX 0
#endif
#define bootLogoTop     180

#ifndef BATTERY_OFF
  #define BatX      200				// X coordinate for batt. ( X  )
  #define BatY      423		// Y cordinate for batt. ( Y  )
  #define BatFS     3		// FontSize for batt. (   )
  #define ProcX     140				// X coordinate for percent ( X   )
  #define ProcY     428		// Y coordinate for percent ( Y   )
  #define ProcFS    2		// FontSize for percent (    )
  #define VoltX      280				// X coordinate for voltage ( X  )
  #define VoltY      428		// Y coordinate for voltage ( Y  )
  #define VoltFS     2		// FontSize for voltage (   )
#endif

/* SROLLS  */                            /* {{ left, top, fontsize, align }, buffsize, uppercase, width, scrolldelay, scrolldelta, scrolltime } */
const ScrollConfig metaConf       PROGMEM = {{ TFT_FRAMEWDT+30, 107, 5, WA_CENTER }, 140, true, MAX_WIDTH-55, 5000, 50, 12 }; //5,12
const ScrollConfig title1Conf     PROGMEM = {{ TFT_FRAMEWDT+10, 158, 3, WA_CENTER }, 140, true, MAX_WIDTH-15, 5000, 30, 12 };
const ScrollConfig title2Conf     PROGMEM = {{ TFT_FRAMEWDT+15, 155, 3, WA_CENTER }, 140, false, MAX_WIDTH-20, 5000, 30, 12 };
const ScrollConfig playlistConf   PROGMEM = {{ TFT_FRAMEWDT, 146, 4, WA_LEFT }, 140, false, MAX_WIDTH, 1000, 30, 12 };
const ScrollConfig apTitleConf    PROGMEM = {{ TFT_FRAMEWDT+10, 107, 4, WA_CENTER }, 140, true, MAX_WIDTH-20, 0, 30, 8 };
const ScrollConfig apSettConf     PROGMEM = {{ TFT_FRAMEWDT+10, 350, 2, WA_CENTER }, 140, false, MAX_WIDTH-20, 0, 30, 8 };
const ScrollConfig weatherConf    PROGMEM = {{ TFT_FRAMEWDT+65, 65, 3, WA_LEFT }, 140, true, MAX_WIDTH-140, 1000, 30, 4 }; //10,4

/* BACKGROUNDS  */                       /* {{ left, top, fontsize, align }, width, height, outlined } */
const FillConfig   metaBGConf     PROGMEM = {{ TFT_FRAMEWDT+10, 105, 0, WA_LEFT }, MAX_WIDTH, 45, false };
const FillConfig   metaBGConfInv  PROGMEM = {{ TFT_FRAMEWDT, 147, 0, WA_LEFT }, MAX_WIDTH, 3, false };
const FillConfig   volbarConf     PROGMEM = {{ TFT_FRAMEWDT+130, DSP_HEIGHT-25, 0, WA_LEFT }, MAX_WIDTH-265, 30, true };
const FillConfig  playlBGConf     PROGMEM = {{ 0, 138, 0, WA_LEFT }, DSP_WIDTH, 36, false };
const FillConfig  heapbarConf     PROGMEM = {{ TFT_FRAMEWDT+10, 147, 0, WA_LEFT }, MAX_WIDTH-27, 3, false };

/* WIDGETS  */                           /* { left, top, fontsize, align } */
const WidgetConfig bootstrConf    PROGMEM = { 0, bootLogoTop+100, 2, WA_CENTER };
const WidgetConfig bitrateConf    PROGMEM = { 0, 45, 1, WA_LEFT };
const WidgetConfig voltxtConf     PROGMEM = { 0, 45, 1, WA_CENTER };
const WidgetConfig  iptxtConf     PROGMEM = { 0, 20, 2, WA_CENTER };
const WidgetConfig   rssiConf     PROGMEM = { 5, 265, 1, WA_RIGHT };
const WidgetConfig   cpuConf      PROGMEM = { 2, 265, 1, WA_LEFT };
const WidgetConfig numConf        PROGMEM = { TFT_FRAMEWDT, 250, 1, WA_CENTER };
const WidgetConfig apNameConf     PROGMEM = { TFT_FRAMEWDT+10, 160, 3, WA_CENTER };
const WidgetConfig apName2Conf    PROGMEM = { TFT_FRAMEWDT+10, 195, 3, WA_CENTER };
const WidgetConfig apPassConf     PROGMEM = { TFT_FRAMEWDT+10, 240, 3, WA_CENTER };
const WidgetConfig apPass2Conf    PROGMEM = { TFT_FRAMEWDT+10, 265, 3, WA_CENTER };
const WidgetConfig  clockConf     PROGMEM = { 70, 250, 52, WA_CENTER };  /* 52 is a fixed font size. do not change */
const WidgetConfig vuConf         PROGMEM = { TFT_FRAMEWDT+73, 310, 2, WA_LEFT };//294

const WidgetConfig bootWdtConf    PROGMEM = { 200, bootLogoTop+130, 2, WA_LEFT }; // left, top, size, align
const ProgressConfig bootPrgConf  PROGMEM = { 90, 9, 4 }; // speed, width, moving part width
const BitrateConfig fullbitrateConf PROGMEM = {{5, 212, 2, WA_LEFT}, 42 };

/* BANDS  */                             /* { onebandwidth, onebandheight, bandsHspace, bandsVspace, numofbands, fadespeed } */
const VUBandsConfig bandsConf     PROGMEM = {  MAX_WIDTH-140, 25, 4, 5, 30, 15}; // lower fadespeed = slower fade

/* STRINGS  */
const char         numtxtFmt[]    PROGMEM = "%d";
//const char           rssiFmt[]    PROGMEM = "WiFi %d";
const char           rssiFmt[]    PROGMEM = "WiFi %d";

const char          iptxtFmt[]    PROGMEM = "%s";
//const char         voltxtFmt[]    PROGMEM = "\023\025%d";
const char         voltxtFmt[]    PROGMEM = "VOL %d";

const char        bitrateFmt[]    PROGMEM = "%d kBs";

/* MOVES  */                             /* { left, top, width } */
const MoveConfig    clockMove     PROGMEM = { 70, 205, MAX_WIDTH /* MAX_WIDTH */ }; // -1 disables move
const MoveConfig   weatherMove    PROGMEM = { TFT_FRAMEWDT+65, 65, MAX_WIDTH-140};
const MoveConfig   weatherMoveVU  PROGMEM = { TFT_FRAMEWDT+65, 65, MAX_WIDTH-140};

/* SPECTRUM ANALYZER  */                 /* Spectrum analyzer config for UEDX48480021 (480x480 round) */
const SpectrumWidgetConfig spectrumConf PROGMEM = {
    .widget = {
        .left = TFT_FRAMEWDT+74,
        .top = 300,
        .textsize = 1,
        .align = WA_CENTER
    },
    .width = 320,          // widget area width for RGB Panel (480 - 2*TFT_FRAMEWDT)
    .height = 110,         // widget area height for RGB Panel (reduced for round display)
    .barWidth = 30,        // bar width for RGB Panel (15*30 + 14*2)
    .barGap = 2,           // gap between bars for RGB Panel
    .orientation = 0,      // horizontal
    .showPeaks = true,     // show peaks for RGB Panel
    .showGrid = true,      // grid for RGB Panel
    .gridColor = 0x39E7,   // gray (optimized for RGB Panel)
    .barColor = 0x07E0,    // green (optimized for RGB Panel)
    .peakColor = 0xF800,   // red (optimized for RGB Panel)
    .bgColor = 0x0000      // black (optimized for RGB Panel)
};

#endif
