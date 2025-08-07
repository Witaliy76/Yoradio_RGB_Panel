#include "spectrum_analyzer.h"
#include "../core/options.h"
#include "esp_heap_caps.h"
#include <math.h>

// Глобальный экземпляр
SpectrumAnalyzer spectrumAnalyzer;

// Простая реализация спектр-анализатора без FFT
// Используем простые полосовые фильтры для моно сигнала

SpectrumAnalyzer::SpectrumAnalyzer() {
    spectrum = nullptr;
    peakHold = nullptr;
    peakTime = nullptr;
    prevSpectrum = nullptr;
    initialized = false;
    dataMutex = nullptr;
}

SpectrumAnalyzer::~SpectrumAnalyzer() {
    if (spectrum) free(spectrum);
    if (peakHold) free(peakHold);
    if (peakTime) free(peakTime);
    if (prevSpectrum) free(prevSpectrum);
    if (dataMutex) vSemaphoreDelete(dataMutex);
}

bool SpectrumAnalyzer::init() {
    // Простая инициализация без FFT
    config.numBands = SPECTRUM_BANDS;
    config.fftSize = SPECTRUM_FFT_SIZE;
    config.sampleRate = 22050; // Примерная частота дискретизации
    config.smoothing = SPECTRUM_SMOOTHING;
    config.peakHoldTime = SPECTRUM_PEAK_HOLD_TIME;
    config.logarithmic = SPECTRUM_LOGARITHMIC;
    config.stereo = SPECTRUM_STEREO;
    config.gain = SPECTRUM_GAIN; // Общее усиление из макроса
    
    return begin();
}

bool SpectrumAnalyzer::begin() {
    // Проверяем наличие PSRAM
    if (!psramFound()) {
        Serial.println("[Spectrum] ERROR: PSRAM not found! Spectrum analyzer requires PSRAM.");
        return false;
    }
    
    Serial.printf("[Spectrum] Initializing simple mono analyzer with %d bands...\n", config.numBands);
    
    // Создаем мьютекс для защиты данных
    dataMutex = xSemaphoreCreateMutex();
    if (!dataMutex) {
        Serial.println("[Spectrum] ERROR: Failed to create data mutex!");
        return false;
    }
    
    // Выделяем память в PSRAM только для спектра
    spectrum = (float*)ps_malloc(config.numBands * sizeof(float));
    peakHold = (float*)ps_malloc(config.numBands * sizeof(float));
    peakTime = (uint32_t*)ps_malloc(config.numBands * sizeof(uint32_t));
    prevSpectrum = (float*)ps_malloc(config.numBands * sizeof(float));
    
    // Выделение памяти для динамического масштабирования
    adaptiveScale = (float*)ps_malloc(config.numBands * sizeof(float));
    maxValues = (float*)ps_malloc(config.numBands * sizeof(float));
    minValues = (float*)ps_malloc(config.numBands * sizeof(float));
    
    // Проверяем выделение памяти
    if (!spectrum || !peakHold || !peakTime || !prevSpectrum || !adaptiveScale || !maxValues || !minValues) {
        Serial.println("[Spectrum] ERROR: Failed to allocate memory in PSRAM!");
        if (spectrum) free(spectrum);
        if (peakHold) free(peakHold);
        if (peakTime) free(peakTime);
        if (prevSpectrum) free(prevSpectrum);
        if (adaptiveScale) free(adaptiveScale);
        if (maxValues) free(maxValues);
        if (minValues) free(minValues);
        if (dataMutex) vSemaphoreDelete(dataMutex);
        spectrum = nullptr;
        peakHold = nullptr;
        peakTime = nullptr;
        prevSpectrum = nullptr;
        adaptiveScale = nullptr;
        maxValues = nullptr;
        minValues = nullptr;
        dataMutex = nullptr;
        return false;
    }
    
    // Инициализация состояния
    sampleCount = 0;
    lastFFTTime = 0;
    debugTime = 0;
    
    // Инициализация массивов
    for (uint8_t i = 0; i < config.numBands; i++) {
        spectrum[i] = 0.0f;
        peakHold[i] = 0.0f;
        peakTime[i] = 0;
        prevSpectrum[i] = 0.0f;
        adaptiveScale[i] = 1.0f;
        maxValues[i] = 0.1f; // Начинаем с небольшого значения
        minValues[i] = 0.0f; // Начинаем с нуля
    }
    
    // Веса для разных частотных полос (улучшены для равномерности)
    float weights[15] = {
        1.2f, 1.5f, 1.8f, 2.0f, 2.2f,  // Низкие частоты (бас)
        2.5f, 2.8f, 3.0f, 2.8f, 2.5f,  // Средние частоты (вокал)
        2.2f, 1.8f, 1.5f, 1.2f, 1.0f   // Высокие частоты (писк)
    };
    
    Serial.printf("[Spectrum] Simple mono analyzer initialized with %d bands\n", config.numBands);
    
    initialized = true;
    return true;
}

void SpectrumAnalyzer::reset() {
    if (dataMutex) {
        vSemaphoreDelete(dataMutex);
        dataMutex = nullptr;
    }
    
    if (spectrum) free(spectrum);
    if (peakHold) free(peakHold);
    if (peakTime) free(peakTime);
    if (prevSpectrum) free(prevSpectrum);
    if (adaptiveScale) free(adaptiveScale);
    if (maxValues) free(maxValues);
    if (minValues) free(minValues);
    
    spectrum = nullptr;
    peakHold = nullptr;
    peakTime = nullptr;
    prevSpectrum = nullptr;
    adaptiveScale = nullptr;
    maxValues = nullptr;
    minValues = nullptr;
    
    initialized = false;
}

void SpectrumAnalyzer::clearData() {
    if (!initialized || !spectrum || !dataMutex) {
        return;
    }
    
    // Захватываем мьютекс для записи данных
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return; // Не можем получить мьютекс
    }
    
    // Сбрасываем все данные спектра
    for (uint8_t i = 0; i < config.numBands; i++) {
        spectrum[i] = 0.0f;
        peakHold[i] = 0.0f;
        peakTime[i] = 0;
        prevSpectrum[i] = 0.0f;
        adaptiveScale[i] = 1.0f;
        maxValues[i] = 0.1f; // Начинаем с небольшого значения
        minValues[i] = 0.0f; // Начинаем с нуля
    }
    
    // Сбрасываем счетчики
    sampleCount = 0;
    lastFFTTime = 0;
    
    // Освобождаем мьютекс
    xSemaphoreGive(dataMutex);
    
    Serial.println("[Spectrum] Data cleared");
}

void SpectrumAnalyzer::processAudio(const int16_t* samples, uint16_t count) {
    if (!initialized || !spectrum || !dataMutex) {
        return;
    }

    // Простая реализация без FFT
    // Используем RMS (Root Mean Square) для каждой полосы частот
    
    uint32_t currentTime = millis();
    
    // Обновляем спектр каждые 50мс (быстрее для лучшей реакции)
    if (currentTime - lastFFTTime < 50) {
        return;
    }
    
    // Захватываем мьютекс для записи данных
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return; // Не можем получить мьютекс, пропускаем обновление
    }
    
    // Явная инициализация массива spectrum
    for (uint8_t i = 0; i < config.numBands; i++) {
        spectrum[i] = 0.0f;
    }
    
    // Простой анализ моно сигнала
    float rms = 0.0f;
    float maxSample = 0.0f;
    
    // Вычисляем RMS и максимальное значение
    for (uint16_t i = 0; i < count; i++) {
        float sample = samples[i] / 32768.0f; // Нормализация к [-1, 1]
        rms += sample * sample;
        if (abs(sample) > maxSample) {
            maxSample = abs(sample);
        }
    }
    rms = sqrt(rms / count);
    
    // Динамическое масштабирование в зависимости от уровня сигнала
    float dynamicGain = 2.0f; // Уменьшаем базовое усиление (было 5.0f)
    if (rms > 0.1f) {
        dynamicGain = 1.0f; // Уменьшаем усиление для громких сигналов (было 2.0f)
    } else if (rms > 0.05f) {
        dynamicGain = 1.5f; // Уменьшаем усиление для средних сигналов (было 3.0f)
    } else if (rms > 0.01f) {
        dynamicGain = 2.0f; // Уменьшаем усиление для тихих сигналов (было 4.0f)
    } else {
        dynamicGain = 2.5f; // Уменьшаем усиление для очень тихих сигналов (было 5.0f)
    }
    
    // Гарантируем, что не выйдем за пределы массива weights
    if (config.numBands > 15) config.numBands = 15;
    float weights[15] = {
        1.2f, 1.5f, 1.8f, 2.0f, 2.2f,  // Низкие частоты (бас)
        2.5f, 2.8f, 3.0f, 2.8f, 2.5f,  // Средние частоты (вокал)
        2.2f, 1.8f, 1.5f, 1.2f, 1.0f   // Высокие частоты (писк)
    };
    
    // Добавляем немного случайности для имитации спектра
    static uint32_t seed = 12345; // Начинаем с ненулевого значения
    for (uint8_t i = 0; i < config.numBands; i++) {
        float weight = (i < 15) ? weights[i] : 1.0f;
        float baseValue = rms * weight;
        
        // Более реалистичное случайное изменение для каждого столбика
        seed = seed * 1103515245 + 12345 + i; // Разный seed для каждого столбика
        float randomFactor = (float)(seed % 100) / 100.0f * 0.4f + 0.6f; // Больше вариаций
        float newValue = baseValue * randomFactor;
        
        // Увеличиваем значения для лучшей видимости
        newValue *= dynamicGain;
        
        // Применяем общее усиление
        newValue *= config.gain;
        
        // Защита от NaN и Inf
        if (isnan(newValue) || isinf(newValue)) {
            newValue = 0.0f;
        }
        
        spectrum[i] = newValue;
        
        // ДИНАМИЧЕСКОЕ МАСШТАБИРОВАНИЕ для каждого столбика
        if (spectrum[i] > 0.0f) {
            // Обновляем максимальные и минимальные значения для этой полосы
            if (spectrum[i] > maxValues[i]) {
                maxValues[i] = spectrum[i];
            }
            if (spectrum[i] < minValues[i] || minValues[i] == 1.0f) {
                minValues[i] = spectrum[i];
            }
            
            // Вычисляем адаптивный масштаб для этой полосы
            float range = maxValues[i] - minValues[i];
            if (range > 0.001f) { // Минимальный диапазон для избежания деления на ноль
                // Нормализуем значение к диапазону [0, 1] для этой полосы
                float normalized = (spectrum[i] - minValues[i]) / range;
                
                // Применяем менее чувствительную степенную кривую
                normalized = pow(normalized, 0.7f); // Менее чувствительная кривая (было 0.3f)
                
                // Уменьшаем диапазон для лучшей динамики
                normalized *= 0.8f; // Уменьшаем диапазон (было 2.0f)
                
                // Ограничиваем значения
                if (normalized < 0.0f) normalized = 0.0f;
                if (normalized > 1.0f) normalized = 1.0f;
                
                spectrum[i] = normalized;
            } else {
                // Если диапазон слишком мал, используем простое масштабирование
                spectrum[i] = spectrum[i] * 1.0f; // Уменьшаем усиление (было 3.0f)
                if (spectrum[i] > 1.0f) spectrum[i] = 1.0f;
            }
        }
        
        // Постепенное затухание максимальных значений для адаптации
        maxValues[i] *= 0.99f; // Еще более быстрое затухание (было 0.995f)
        if (maxValues[i] < 0.01f) maxValues[i] = 0.01f; // Минимальный порог
        
        // Постепенное увеличение минимальных значений
        minValues[i] *= 1.01f; // Еще более быстрое увеличение (было 1.005f)
        if (minValues[i] > 0.2f) minValues[i] = 0.2f; // Еще больше снижаем порог (было 0.3f)
        
        // Принудительное сжатие диапазона при тихих моментах
        if (rms < 0.02f) { // Расширяем диапазон тихих сигналов (было 0.01f)
            maxValues[i] *= 0.90f; // Очень быстрое затухание (было 0.95f)
            minValues[i] *= 1.05f; // Очень быстрое увеличение (было 1.02f)
        } else if (rms < 0.05f) { // Добавляем средний диапазон (было 0.02f)
            maxValues[i] *= 0.95f; // Быстрое затухание (было 0.98f)
            minValues[i] *= 1.02f; // Быстрое увеличение (было 1.01f)
        }
        
        // Периодическое принудительное сжатие для всех полос
        static uint32_t lastCompressionTime = 0;
        if (currentTime - lastCompressionTime > 2000) { // Каждые 2 секунды
            lastCompressionTime = currentTime;
            // Сжимаем диапазон всех полос
            maxValues[i] *= 0.85f; // Сильное затухание
            minValues[i] *= 1.03f; // Сильное увеличение
        }
        
        // Дополнительное сглаживание для стабильности
        spectrum[i] = spectrum[i] * 0.7f + prevSpectrum[i] * 0.3f; // Уменьшаем сглаживание для быстрой реакции
        
        // Защита от NaN и Inf
        if (isnan(spectrum[i]) || isinf(spectrum[i])) {
            spectrum[i] = 0.0f;
        }
        
        // Ограничиваем значения
        if (spectrum[i] < 0.0f) spectrum[i] = 0.0f;
        if (spectrum[i] > 1.0f) spectrum[i] = 1.0f;
        
        if (spectrum[i] > peakHold[i]) {
            peakHold[i] = spectrum[i];
            peakTime[i] = currentTime;
        } else if (currentTime - peakTime[i] > config.peakHoldTime) {
            peakHold[i] *= 0.90f;
            if (peakHold[i] < spectrum[i]) {
                peakHold[i] = spectrum[i];
            }
        }
        
        // Защита пиковых значений от NaN
        if (isnan(peakHold[i]) || isinf(peakHold[i])) {
            peakHold[i] = 0.0f;
        }
    }
    lastFFTTime = currentTime;
    
    // Освобождаем мьютекс
    xSemaphoreGive(dataMutex);
    
    // Отладочная информация удалена для чистоты вывода
}

// Простые функции для моно индикатора
float* SpectrumAnalyzer::getSpectrum() {
    if (!initialized || !spectrum || !dataMutex) {
        return nullptr;
    }
    
    // Захватываем мьютекс для чтения данных
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(5)) != pdTRUE) {
        return nullptr; // Не можем получить мьютекс
    }
    
    // Освобождаем мьютекс сразу после получения указателя
    xSemaphoreGive(dataMutex);
    
    return spectrum;
}

float* SpectrumAnalyzer::getPeakHold() {
    if (!initialized || !peakHold || !dataMutex) {
        return nullptr;
    }
    
    // Возвращаем указатель без захвата мьютекса
    // Виджет должен использовать copySpectrum для безопасного доступа
    return peakHold;
}

uint8_t SpectrumAnalyzer::getNumBands() {
    return config.numBands;
}

bool SpectrumAnalyzer::isInitialized() {
    return initialized;
}

// Безопасное копирование данных спектра
bool SpectrumAnalyzer::copySpectrum(float* dest, uint8_t maxBands) {
    if (!initialized || !spectrum || !dataMutex || !dest) {
        return false;
    }
    
    // Захватываем мьютекс для чтения данных
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return false; // Не можем получить мьютекс
    }
    
    // Копируем данные с защитой от NaN
    uint8_t bandsToCopy = (config.numBands < maxBands) ? config.numBands : maxBands;
    for (uint8_t i = 0; i < bandsToCopy; i++) {
        if (isnan(spectrum[i]) || isinf(spectrum[i])) {
            dest[i] = 0.0f;
        } else {
            dest[i] = spectrum[i];
        }
    }
    
    // Освобождаем мьютекс
    xSemaphoreGive(dataMutex);
    
    return true;
}

// Безопасное копирование пиковых значений
bool SpectrumAnalyzer::copyPeakHold(float* dest, uint8_t maxBands) {
    if (!initialized || !peakHold || !dataMutex || !dest) {
        return false;
    }
    
    // Захватываем мьютекс для чтения данных
    if (xSemaphoreTake(dataMutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return false; // Не можем получить мьютекс
    }
    
    // Копируем данные с защитой от NaN
    uint8_t bandsToCopy = (config.numBands < maxBands) ? config.numBands : maxBands;
    for (uint8_t i = 0; i < bandsToCopy; i++) {
        if (isnan(peakHold[i]) || isinf(peakHold[i])) {
            dest[i] = 0.0f;
        } else {
            dest[i] = peakHold[i];
        }
    }
    
    // Освобождаем мьютекс
    xSemaphoreGive(dataMutex);
    
    return true;
}

// Управление общим усилением
void SpectrumAnalyzer::setGain(float gain) {
    if (gain < 0.1f) gain = 0.1f; // Минимальное усиление
    if (gain > 10.0f) gain = 10.0f; // Максимальное усиление
    config.gain = gain;
}

float SpectrumAnalyzer::getGain() {
    return config.gain;
} 