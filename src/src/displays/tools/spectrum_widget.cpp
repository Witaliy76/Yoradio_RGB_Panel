#include "spectrum_widget.h"
#include "../core/options.h"
#include "GFX_Canvas_screen.h"
#include <algorithm>

// Виджет спектр-анализатора оптимизирован для RGB Panel ESP32
// Поддерживает 480x480 разрешение с плавной анимацией

// Объявление глобальной переменной gfx из displayxxxxx.cpp
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
    _lastEnergy = -1;
    _lastDrawMs = 0;
    // Инициализируем предыдущие высоты как -1 (первый запуск)
    for (uint8_t i = 0; i < 15; i++) {
        _prevBarH[i] = -1;
        _prevPeakH[i] = -1;
    }
    
    // Инициализация базового виджета для RGB Panel
    Widget::init(conf.widget, conf.barColor, conf.bgColor);
    // Очистим область сразу, чтобы при старте не было артефактов
    // Но не для UEDX48480021, чтобы не затирать дату и другие виджеты
    #if DSP_MODEL != DSP_UEDX48480021
        _clear();
    #endif
}

void SpectrumWidget::loop() {
    if (!(_active && !_locked)) return;
    _draw(); // Отрисовка спектра для RGB Panel
}

void SpectrumWidget::setActive(bool act, bool clr) {
    if (act) {
        // При активации сбрасываем состояние для чистой перерисовки
        _lastEnergy = -1;
        _lastDrawMs = 0;
        // Сбрасываем предыдущие высоты для принудительной перерисовки
        for (uint8_t i = 0; i < 15; i++) {
            _prevBarH[i] = -1;
            _prevPeakH[i] = -1;
        }
    }
    Widget::setActive(act, clr);
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
    
    // Вычисляем энергию спектра для проверки изменений (анти-мерцание)
    float energy = 0.0f;
    for (uint8_t i = 0; i < numBands && i < 15; i++) {
        if (!isnan(spectrumCopy[i]) && !isinf(spectrumCopy[i])) {
            energy += spectrumCopy[i];
        }
    }
    int energyInt = (int)(energy * 1000.0f); // Преобразуем в int для сравнения
    
    // Проверка изменений перед отрисовкой (анти-мерцание)
    const int THRESH_ENERGY = 50; // Порог изменения энергии (0.05 * 1000)
    uint32_t now = millis();
    if (_lastEnergy >= 0 && abs(energyInt - _lastEnergy) < THRESH_ENERGY && (now - _lastDrawMs) < 100) {
        return; // Изменения слишком малы, не рисуем
    }
    _lastEnergy = energyInt;
    _lastDrawMs = now;
    
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
    // pad для очистки по Y: 1px достаточно для тонкой линии,
    // 2px если включена вторая линия толщины.
    // +1px если включены боковины (там вертикали).
    const uint16_t CAP_PAD =
        (SPECTRUM_PEAK_THICKNESS >= 2 ? 2 : 1) +
        (SPECTRUM_PEAK_SIDES ? 1 : 0);
    
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
        
        // Вычисляем высоту пика ДО очистки, чтобы учесть её при очистке
        uint16_t peakHeight = 0;
        if (_showPeaks && peak > value) {
            peakHeight = (uint16_t)(peak * _config.height);
            if (peakHeight < 1) peakHeight = 1;
            if (peakHeight > _config.height) peakHeight = _config.height;
        }
        
        // Получаем предыдущие высоты (если -1, считаем равными 0 для первого запуска)
        uint16_t oldBarH = (_prevBarH[i] < 0) ? 0 : (uint16_t)_prevBarH[i];
        uint16_t oldPeakH = (_prevPeakH[i] < 0) ? 0 : (uint16_t)_prevPeakH[i];
        
        // Вычисляем максимальную высоту для очистки снизу: учитываем старые и новые значения баров и пиков
        uint16_t oldPeakHWithPad = (oldPeakH > 0) ? (oldPeakH + CAP_PAD) : 0;
        uint16_t peakHWithPad = (peakHeight > 0) ? (peakHeight + CAP_PAD) : 0;
        uint16_t maxH = std::max(oldBarH, std::max(barHeight, std::max(oldPeakHWithPad, peakHWithPad)));
        if (maxH > _config.height) maxH = _config.height;
        
        // Очищаем нижнюю область столбика от максимальной высоты до низа виджета
        if (gfx && maxH > 0) {
            uint16_t clearY = _config.widget.top + _config.height - maxH;
            // Clamp: если clearY меньше top, устанавливаем в top
            if (clearY < _config.widget.top) clearY = _config.widget.top;
            // Вычисляем высоту очищаемой области
            uint16_t clearH = (_config.widget.top + _config.height) - clearY;
            if (clearH > 0) {
                gfxFillRect(gfx, x, clearY, _config.barWidth, clearH, _config.bgColor);
            }
        }
        
        _drawBar(x, _config.barWidth, barHeight, value, peak);
        
        // Сохраняем текущие высоты для следующего кадра
        _prevBarH[i] = barHeight;
        _prevPeakH[i] = peakHeight;
    }
    
    // NOTE: Removed local flush() to avoid mid-frame flush conflicts
    // Canvas will be flushed once at end of frame by display loop
    // if (gfx) {
    //     gfx->flush(); // Обновление RGB Panel для плавной анимации
    // }
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
    // Сбрасываем состояние для следующей перерисовки
    _lastEnergy = -1;
    _lastDrawMs = 0;
    // Сбрасываем предыдущие высоты
    for (uint8_t i = 0; i < 15; i++) {
        _prevBarH[i] = -1;
        _prevPeakH[i] = -1;
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
    
    // Рисуем столбик
    if (gfx) {
        #if SPECTRUM_GRADIENT
            // Градиентная заливка для RGB Panel (плавные переходы цветов)
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
        #else
            // Простая заливка SOLID цветом (быстро, четко для QSPI)
            // Выбираем цвет в зависимости от высоты столбика
            uint16_t barColor = _barColor; // Зеленый по умолчанию
            float heightPercent = (float)barHeight / _config.height;
            
            if (heightPercent > 0.90f) {       // Верхние 10% - красный
                barColor = _peakColor;         // Красный
            } else if (heightPercent > 0.50f) { // От 50% до 90% - желтый
                barColor = 0xFFE0;             // Желтый
            }
            // До 50% остается зеленый (_barColor)
            
            // Рисуем столбик одним цветом
            gfxFillRect(gfx, x, baseY, width, barHeight, barColor);
        #endif

        // Рисуем пиковое значение если включено
        if (_showPeaks && peak > value) {
            uint16_t peakHeight = (uint16_t)(peak * _config.height);
            if (peakHeight < 1) peakHeight = 1;
            if (peakHeight > _config.height) peakHeight = _config.height;

            uint16_t peakY = _config.widget.top + _config.height - peakHeight;

            // 1) Тонкая шапочка / Thin peak cap
            // Всегда рисуем в пределах barWidth: x .. x+width-1
            gfxDrawLine(gfx, x, peakY, x + width - 1, peakY, _peakColor);

#if SPECTRUM_PEAK_THICKNESS >= 2
            // Вторая линия толщины (лучше вверх, чтобы не лезть вниз в столбик)
            // Second thickness line (better upward to avoid going into the bar)
            if (peakY > _config.widget.top) {
                gfxDrawLine(gfx, x, peakY - 1, x + width - 1, peakY - 1, _peakColor);
            }
#endif

#if SPECTRUM_PEAK_SIDES
            // 2) Боковины (если нужны) / Vertical sides (if needed)
            // Вертикальные линии, идущие ВНИЗ от верхней горизонтальной линии (визуально как скобка [ перевернутая)
            if (width > 2) {
#if SPECTRUM_PEAK_THICKNESS >= 2
                // Для THICKNESS=2: боковины идут вниз от верхней линии (peakY-1) на 2-3 пикселя
                uint16_t topLineY = (peakY > _config.widget.top) ? (peakY - 1) : peakY;
                uint16_t bottomY = (topLineY + 2 < (_config.widget.top + _config.height)) ? (topLineY + 2) : (_config.widget.top + _config.height - 1);
                gfxDrawLine(gfx, x, topLineY, x, bottomY, _peakColor);
                gfxDrawLine(gfx, x + width - 1, topLineY, x + width - 1, bottomY, _peakColor);
#else
                // Для THICKNESS=1: боковины идут вниз от основной линии (peakY) на 2-3 пикселя
                uint16_t bottomY = (peakY + 2 < (_config.widget.top + _config.height)) ? (peakY + 2) : (_config.widget.top + _config.height - 1);
                gfxDrawLine(gfx, x, peakY, x, bottomY, _peakColor);
                gfxDrawLine(gfx, x + width - 1, peakY, x + width - 1, bottomY, _peakColor);
#endif
            }
#endif
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