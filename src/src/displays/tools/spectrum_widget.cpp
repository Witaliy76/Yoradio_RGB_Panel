#include "spectrum_widget.h"
#include "../core/options.h"
#include "GFX_Canvas_screen.h"

// Виджет спектр-анализатора оптимизирован для RGB Panel ESP32-4848S040
// Поддерживает 480x480 разрешение с плавной анимацией

// Объявление глобальной переменной gfx из displayAXS15231B.cpp
extern Arduino_Canvas *gfx;

SpectrumWidget::~SpectrumWidget() {
    // Деструктор
}

void SpectrumWidget::init(SpectrumWidgetConfig conf) {
    _config = conf;
    _barColor = conf.barColor;
    _peakColor = conf.peakColor;
    _showPeaks = conf.showPeaks;
    _showGrid = conf.showGrid;
    
    // Инициализация базового виджета для RGB Panel
    Widget::init(conf.widget, conf.barColor, conf.bgColor);
}

void SpectrumWidget::loop() {
    if (_active && !_locked) {
        _draw(); // Отрисовка спектра для RGB Panel
    } else if (!_active) {
        // При деактивации очищаем экран для RGB Panel
        _clear();
        if (gfx) {
            gfx->flush(); // Обновление RGB Panel
        }
    }
}

void SpectrumWidget::_draw() {
    if (!spectrumAnalyzer.isInitialized()) {
        return;
    }
    
    // Безопасное копирование данных спектра для RGB Panel
    uint8_t numBands = spectrumAnalyzer.getNumBands();
    float spectrumCopy[15]; // Максимум 15 полос для RGB Panel
    float peakHoldCopy[15];
    
    if (!spectrumAnalyzer.copySpectrum(spectrumCopy, 15)) {
        return;
    }
    
    // Копируем пиковые значения для RGB Panel
    if (!spectrumAnalyzer.copyPeakHold(peakHoldCopy, 15)) {
        for (uint8_t i = 0; i < numBands && i < 15; i++) {
            peakHoldCopy[i] = 0.0f;
        }
    }
    
    // Очищаем область виджета для RGB Panel
    _clear();
    
    // Рисуем сетку если включена для RGB Panel
    if (_showGrid) {
        _drawGrid();
    }
    
    // Вычисляем размеры полос для RGB Panel 480x480
    uint16_t totalBars = numBands;
    if (totalBars > _config.width / (_config.barWidth + _config.barGap)) {
        totalBars = _config.width / (_config.barWidth + _config.barGap);
    }
    
    uint16_t startX = _config.widget.left;
    
    // Рисуем полосы спектра для RGB Panel
    for (uint16_t i = 0; i < totalBars && i < 15; i++) {
        uint16_t x = startX + i * (_config.barWidth + _config.barGap);
        float value = spectrumCopy[i];
        float peak = peakHoldCopy[i];
        
        // Защита от NaN и Inf для RGB Panel
        if (isnan(value) || isinf(value)) value = 0.0f;
        if (isnan(peak) || isinf(peak)) peak = 0.0f;
        
        // Ограничиваем значения для RGB Panel (логарифмическая нормализация дает диапазон [0, 1])
        if (value > 1.0f) value = 1.0f;
        if (peak > 1.0f) peak = 1.0f;
        
        // Вычисляем высоту столбика напрямую из значения для RGB Panel
        uint16_t barHeight = (uint16_t)(value * _config.height);
        
        // Минимальная высота для видимости слабых сигналов на RGB Panel
        if (barHeight < 2 && value > 0.0f) {
            barHeight = 2;
        }
        
        _drawBar(x, _config.barWidth, barHeight, value, peak);
    }
    
    // Обновляем экран RGB Panel (Canvas путь)
    if (gfx) {
        gfx->flush(); // Обновление RGB Panel для плавной анимации
    }
}

void SpectrumWidget::_clear() {
    // Очищаем область виджета для RGB Panel (Canvas путь)
    if (gfx) {
        gfxFillRect(gfx, _config.widget.left, _config.widget.top,
                    _config.width, _config.height, _config.bgColor);
    }
}

void SpectrumWidget::_drawBar(uint16_t x, uint16_t width, uint16_t barHeight, float value, float peak) {
    // Проверяем границы виджета для RGB Panel
    if (x >= _config.widget.left + _config.width) return;
    
    // Защита от NaN и Inf для RGB Panel
    if (isnan(value) || isinf(value)) value = 0.0f;
    if (isnan(peak) || isinf(peak)) peak = 0.0f;
    
    // Минимальная высота для видимости на RGB Panel
    if (barHeight < 2) barHeight = 2;
    
    // Ограничиваем высоту размером виджета для RGB Panel
    if (barHeight > _config.height) barHeight = _config.height;
    
    // Вычисляем Y-координату для отрисовки снизу вверх на RGB Panel
    uint16_t baseY = _config.widget.top + _config.height - barHeight;
    
    // Проверяем, что полоса не выходит за границы виджета RGB Panel
    if (baseY < _config.widget.top) {
        baseY = _config.widget.top;
        barHeight = _config.height;
    }
    
    // Рисуем столбик с градиентом для RGB Panel
    uint16_t barColor = _barColor; // Зеленый по умолчанию для RGB Panel
    
    // Вычисляем процент высоты столбика от максимальной высоты виджета RGB Panel
    float heightPercent = (float)barHeight / _config.height;
    
    if (heightPercent > 0.90f) { // Верхние 10% - красный для RGB Panel
        barColor = _peakColor; // Красный для RGB Panel
    } else if (heightPercent > 0.50f) { // От 50% до 90% - желтый для RGB Panel
        barColor = 0xFFE0; // Желтый для RGB Panel
    }
    // До 50% остается зеленый (_barColor) для RGB Panel
    
    if (gfx) {
        gfxFillRect(gfx, x, baseY, width, barHeight, barColor);

        // Рисуем пиковое значение если включено для RGB Panel
        if (_showPeaks && peak > value) {
            uint16_t peakHeight = (uint16_t)(peak * _config.height);
            if (peakHeight < 1) peakHeight = 1;
            if (peakHeight > _config.height) peakHeight = _config.height;

            uint16_t peakY = _config.widget.top + _config.height - peakHeight;

            // Рисуем толстую линию пика для RGB Panel
            // Основная горизонтальная линия для RGB Panel
            gfxDrawLine(gfx, x, peakY, x + width, peakY, _peakColor);

            // Дополнительные линии для толщины на RGB Panel
            if (peakY > 0) {
                gfxDrawLine(gfx, x, peakY - 1, x + width, peakY - 1, _peakColor);
            }
            if (peakY < _config.height - 1) {
                gfxDrawLine(gfx, x, peakY + 1, x + width, peakY + 1, _peakColor);
            }

            // Вертикальные линии по краям для лучшей видимости на RGB Panel
            if (width > 2) {
                gfxDrawLine(gfx, x, peakY - 1, x, peakY + 1, _peakColor);
                gfxDrawLine(gfx, x + width - 1, peakY - 1, x + width - 1, peakY + 1, _peakColor);
            }
        }
    }
}

void SpectrumWidget::_drawGrid() {
    // Рисуем горизонтальную сетку для RGB Panel
    uint16_t gridSpacing = _config.height / 4; // 4 линии сетки для RGB Panel
    
    if (gfx) {
        for (uint8_t i = 1; i < 4; i++) {
            uint16_t y = _config.widget.top + i * gridSpacing;
            gfxDrawLine(gfx, _config.widget.left, y,
                        _config.widget.left + _config.width, y, _config.gridColor);
        }
    }
} 