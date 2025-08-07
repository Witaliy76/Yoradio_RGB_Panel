#ifndef SPECTRUM_AXS15231B_H
#define SPECTRUM_AXS15231B_H

#include "../tools/spectrum_widget.h"

// Конфигурация Spectrum Widget для AXS15231B
static SpectrumWidgetConfig spectrumConf = {
    .widget = {
        .left = 10,           // Отступ слева
        .top = 250,           // Отступ сверху
        .textsize = 1,        // Размер текста (не используется)
        .align = WA_LEFT      // Выравнивание (не используется)
    },
    .width = 300,             // Ширина виджета
    .height = 120,             // Высота виджета
    .barWidth = 18,            // Ширина полосы
    .barGap = 2,              // Расстояние между полосами
    .orientation = 0,         // 0 - горизонтальный
    .showPeaks = true,        // Показывать пиковые значения
    .showGrid = true,         // Показывать сетку
    .gridColor = 0x7BEF,      // Цвет сетки (серый)
    .barColor = 0x07E0,       // Цвет полос (зеленый)
    .peakColor = 0xF800,      // Цвет пиков (красный)
    .bgColor = 0x0000         // Цвет фона (черный)
};

#endif 