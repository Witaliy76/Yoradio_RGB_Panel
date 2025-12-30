#ifndef SPECTRUM_WIDGET_H
#define SPECTRUM_WIDGET_H

// Виджет спектр-анализатора оптимизирован для RGB Panel ESP32-4848S040
// Поддерживает 480x480 разрешение с плавной анимацией

#include "../widgets/widgets.h"
#include "spectrum_analyzer.h"

// Настройки толщины пика (peak cap) / Peak cap thickness settings
#ifndef SPECTRUM_PEAK_THICKNESS
  #define SPECTRUM_PEAK_THICKNESS 2   // 1 или 2 (рекомендуем 1) / 1 or 2 (recommend 1)
#endif

#ifndef SPECTRUM_PEAK_SIDES
  #define SPECTRUM_PEAK_SIDES 1       // 0 = без вертикальных боковин, 1 = с боковинами / 0 = no vertical sides, 1 = with sides
#endif

// Конфигурация виджета для RGB Panel
struct SpectrumWidgetConfig {
    WidgetConfig widget;        // Базовая конфигурация виджета
    uint16_t width;            // Ширина виджета (оптимизирована для 480px)
    uint16_t height;           // Высота виджета (оптимизирована для RGB Panel)
    uint8_t barWidth;          // Ширина полосы (настроена для 480x480)
    uint8_t barGap;            // Расстояние между полосами
    uint8_t orientation;       // 0-горизонтальный, 1-вертикальный
    bool showPeaks;            // Показывать пиковые значения
    bool showGrid;             // Показывать сетку
    uint16_t gridColor;        // Цвет сетки (оптимизирован для RGB Panel)
    uint16_t barColor;         // Цвет полос (оптимизирован для RGB Panel)
    uint16_t peakColor;        // Цвет пиков (оптимизирован для RGB Panel)
    uint16_t bgColor;          // Цвет фона (оптимизирован для RGB Panel)
};

class SpectrumWidget: public Widget {
public:
    SpectrumWidget() {}
    SpectrumWidget(SpectrumWidgetConfig conf) { init(conf); }
    ~SpectrumWidget();
    
    void init(SpectrumWidgetConfig conf);
    void loop();
    void setActive(bool act, bool clr=false) override;
    
    // Настройки отображения для RGB Panel
    void setBarColor(uint16_t color) { _barColor = color; }
    void setPeakColor(uint16_t color) { _peakColor = color; }
    void setShowPeaks(bool show) { _showPeaks = show; }
    void setShowGrid(bool show) { _showGrid = show; }
    
protected:
    SpectrumWidgetConfig _config;
    uint16_t _barColor, _peakColor;
    bool _showPeaks, _showGrid;
    // Для предотвращения мерцания - проверка изменений перед отрисовкой
    int _lastEnergy = -1;
    uint32_t _lastDrawMs = 0;
    // Хранение предыдущих высот баров для правильной очистки "шапочек" пиков
    int16_t _prevBarH[15];
    // Хранение предыдущих высот пиков для гарантированной очистки старых "шапочек"
    int16_t _prevPeakH[15];
    
    // Внутренние методы для RGB Panel
    void _draw();
    void _clear();
    void _drawBar(uint16_t x, uint16_t width, uint16_t barHeight, float value, float peak);
    void _drawGrid();
    
    // Музыкальные эффекты для RGB Panel
    uint16_t _applyGradient(uint16_t baseColor, float progress);
    uint16_t _blendColors(uint16_t color1, uint16_t color2, float ratio);
    uint16_t _darkenColor(uint16_t color, float factor);
    uint16_t _brightenColor(uint16_t color, float factor);
};

#endif 