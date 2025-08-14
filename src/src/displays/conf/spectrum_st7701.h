/*************************************************************************************
    ST7701 480x480 Spectrum Analyzer configuration file.
    Based on spectrum_axs15231b.h
    Оптимизировано для RGB Panel ESP32-4848S040
*************************************************************************************/

#ifndef spectrum_st7701_h
#define spectrum_st7701_h

#include "../tools/spectrum_widget.h"

// Конфигурация виджета спектра для ST7701 (480x480) RGB Panel
// Адаптирована под SpectrumWidgetConfig и оптимизирована для ESP32-4848S040
static SpectrumWidgetConfig spectrumConf = {
    .widget = {
        .left = TFT_FRAMEWDT,
        .top = 294,
        .textsize = 1,
        .align = WA_LEFT
    },
    .width = 480,          // ширина области виджета для RGB Panel (480 - 2*TFT_FRAMEWDT)
    .height = 120,         // высота области виджета для RGB Panel
    .barWidth = 30,        // ширина полосы для RGB Panel (15*30 + 14*2 = 478)
    .barGap = 2,           // зазор между полосами для RGB Panel
    .orientation = 0,      // горизонтальный
    .showPeaks = true,     // отображать пики для RGB Panel
    .showGrid = true,      // сетка для RGB Panel
    .gridColor = 0x39E7,   // серый (оптимизирован для RGB Panel)
    .barColor = 0x07E0,    // зелёный (оптимизирован для RGB Panel)
    .peakColor = 0xF800,   // красный (оптимизирован для RGB Panel)
    .bgColor = 0x0000      // чёрный (оптимизирован для RGB Panel)
};

#endif
