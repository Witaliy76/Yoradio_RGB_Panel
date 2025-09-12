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
    // Очистим область сразу, чтобы при старте не было артефактов
    _clear();
}

void SpectrumWidget::loop() {
    if (!(_active && !_locked)) return;
    _draw(); // Отрисовка спектра для RGB Panel
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
        // При деактивации расширяем очистку вниз на пару пикселей,
        // чтобы убрать возможные остатки при переключении SA -> VU
        uint16_t extra = 10;
        gfxFillRect(gfx, _config.widget.left, _config.widget.top,
                    _config.width, (uint16_t)(_config.height + extra), _config.bgColor);//_config.bgColor
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
    
    // Рисуем столбик с красивым градиентом для RGB Panel
    if (gfx) {
        // Создаем градиент от темного к яркому для каждой полосы
        for (uint16_t y = 0; y < barHeight; y++) {
            // Вычисляем прогресс по высоте (0.0 - 1.0)
            float progress = (float)y / barHeight;
            
            // Применяем нелинейную кривую для более естественного градиента
            float gradientProgress = pow(progress, 0.7f);
            
            // Базовый цвет в зависимости от высоты
            uint16_t baseColor;
            float heightPercent = (float)barHeight / _config.height;
            
            if (heightPercent > 0.85f) { // Верхние 15% - красный
                baseColor = _peakColor;
            } else if (heightPercent > 0.60f) { // От 60% до 85% - оранжевый
                baseColor = 0xFD20; // Красивый оранжевый
            } else if (heightPercent > 0.35f) { // От 35% до 60% - желтый
                baseColor = 0xFFE0; // Яркий желтый
            } else { // До 35% - зеленый
                baseColor = _barColor;
            }
            
            // Применяем градиент с затемнением внизу и осветлением вверху
            uint16_t gradientColor = _applyGradient(baseColor, gradientProgress);
            
            // Рисуем одну линию градиента
            gfxDrawLine(gfx, x, baseY + y, x + width - 1, baseY + y, gradientColor);
        }

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

// Применяем красивый градиент для RGB Panel
uint16_t SpectrumWidget::_applyGradient(uint16_t baseColor, float progress) {
    // Создаем градиент от темного к яркому
    uint16_t darkColor = _darkenColor(baseColor, 0.6f);
    uint16_t brightColor = _brightenColor(baseColor, 0.3f);
    
    // Смешиваем цвета в зависимости от прогресса
    return _blendColors(darkColor, brightColor, progress);
}

// Смешиваем два цвета для RGB Panel
uint16_t SpectrumWidget::_blendColors(uint16_t color1, uint16_t color2, float ratio) {
    // Извлекаем RGB компоненты
    uint8_t r1 = (color1 >> 11) & 0x1F;
    uint8_t g1 = (color1 >> 5) & 0x3F;
    uint8_t b1 = color1 & 0x1F;
    
    uint8_t r2 = (color2 >> 11) & 0x1F;
    uint8_t g2 = (color2 >> 5) & 0x3F;
    uint8_t b2 = color2 & 0x1F;
    
    // Смешиваем компоненты
    uint8_t r = (uint8_t)(r1 * (1.0f - ratio) + r2 * ratio);
    uint8_t g = (uint8_t)(g1 * (1.0f - ratio) + g2 * ratio);
    uint8_t b = (uint8_t)(b1 * (1.0f - ratio) + b2 * ratio);
    
    // Собираем цвет обратно
    return (r << 11) | (g << 5) | b;
}

// Затемняем цвет для RGB Panel
uint16_t SpectrumWidget::_darkenColor(uint16_t color, float factor) {
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;
    
    r = (uint8_t)(r * factor);
    g = (uint8_t)(g * factor);
    b = (uint8_t)(b * factor);
    
    return (r << 11) | (g << 5) | b;
}

// Осветляем цвет для RGB Panel
uint16_t SpectrumWidget::_brightenColor(uint16_t color, float factor) {
    uint8_t r = (color >> 11) & 0x1F;
    uint8_t g = (color >> 5) & 0x3F;
    uint8_t b = color & 0x1F;
    
    r = (uint8_t)(r + (31 - r) * factor);
    g = (uint8_t)(g + (63 - g) * factor);
    b = (uint8_t)(b + (31 - b) * factor);
    
    return (r << 11) | (g << 5) | b;
} 