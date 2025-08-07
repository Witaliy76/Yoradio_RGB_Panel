#ifndef SPECTRUM_ANALYZER_H
#define SPECTRUM_ANALYZER_H

#include "Arduino.h"
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Универсальная структура конфигурации
struct SpectrumConfig {
    uint8_t numBands;           // SPECTRUM_BANDS
    uint16_t fftSize;           // SPECTRUM_FFT_SIZE
    uint32_t sampleRate;        // Частота дискретизации
    float smoothing;            // SPECTRUM_SMOOTHING
    float peakHoldTime;         // SPECTRUM_PEAK_HOLD_TIME
    bool logarithmic;           // SPECTRUM_LOGARITHMIC
    bool stereo;                // SPECTRUM_STEREO
    float gain;                 // Общее усиление спектра (1.0 = без усиления)
};

class SpectrumAnalyzer {
private:
    // Конфигурация
    struct {
        uint8_t numBands;
        uint16_t fftSize;
        uint32_t sampleRate;
        float smoothing;
        float peakHoldTime;
        bool logarithmic;
        bool stereo;
        float gain;                 // Общее усиление спектра (1.0 = без усиления)
    } config;
    
    // Данные спектра
    float* spectrum;
    float* peakHold;
    uint32_t* peakTime;
    float* prevSpectrum;
    
    // Динамическое масштабирование
    float* adaptiveScale;      // Адаптивный масштаб для каждой полосы
    float* maxValues;          // Максимальные значения для каждой полосы
    float* minValues;          // Минимальные значения для каждой полосы
    
    // Состояние
    uint16_t sampleCount;
    uint32_t lastFFTTime;
    uint32_t debugTime;
    bool initialized;
    
    // Защита от гонки данных
    SemaphoreHandle_t dataMutex;
    
    // Внутренние функции
    void reset();
    
public:
    SpectrumAnalyzer();
    ~SpectrumAnalyzer();
    
    // Инициализация
    bool init();
    bool begin();
    
    // Обработка аудио
    void processAudio(const int16_t* samples, uint16_t count);
    
    // Сброс данных спектра
    void clearData();
    
    // Получение данных (с защитой)
    float* getSpectrum();
    float* getPeakHold();
    uint8_t getNumBands();
    bool isInitialized();
    
    // Безопасное копирование данных
    bool copySpectrum(float* dest, uint8_t maxBands);
    bool copyPeakHold(float* dest, uint8_t maxBands);
    
    // Управление усилением
    void setGain(float gain);
    float getGain();
};

// Глобальный экземпляр
extern SpectrumAnalyzer spectrumAnalyzer;

#endif 