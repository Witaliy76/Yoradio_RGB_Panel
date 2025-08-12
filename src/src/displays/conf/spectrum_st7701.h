/*************************************************************************************
    ST7701 480x480 Spectrum Analyzer configuration file.
    Based on spectrum_axs15231b.h
*************************************************************************************/

#ifndef spectrum_st7701_h
#define spectrum_st7701_h

#include "../tools/spectrum_widget.h"

// Конфигурация для Spectrum Analyzer на ST7701 (480x480)
const SpectrumConfig spectrumConf PROGMEM = {
    TFT_FRAMEWDT,    // left
    294,             // top (VU position from displayST7701conf.h)
    472,             // width (MAX_WIDTH)
    35,              // height
    15,              // bands
    2,               // spacing between bands
    true,            // show peaks
    500,             // peak hold time (ms)
    0x07E0,          // color (green)
    0xF800,          // peak color (red)
    0x0000           // background color (black)
};

#endif
