/*************************************************************************************
    ST7701 480x480 Spectrum Analyzer configuration file.
    Based on spectrum_axs15231b.h
*************************************************************************************/

#ifndef spectrum_st7701_h
#define spectrum_st7701_h

#include "../tools/spectrum_widget.h"

// Конфигурация виджета спектра для ST7701 (480x480)
// Адаптирована под SpectrumWidgetConfig
static SpectrumWidgetConfig spectrumConf = {
    .widget = {
        .left = TFT_FRAMEWDT,
        .top = 294,
        .textsize = 1,
        .align = WA_LEFT
    },
    .width = 480,          // ширина области виджета (480 - 2*TFT_FRAMEWDT)
    .height = 120,         // высота области виджета
    .barWidth = 30,        // ширина полосы (15*24 + 14*8 = 472)
    .barGap = 2,           // зазор между полосами
    .orientation = 0,      // горизонтальный
    .showPeaks = true,     // отображать пики
    .showGrid = true,      // сетка
    .gridColor = 0x39E7,   // серый
    .barColor = 0x07E0,    // зелёный
    .peakColor = 0xF800,   // красный
    .bgColor = 0x0000      // чёрный
};

#endif
