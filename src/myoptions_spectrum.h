// Пример конфигурации для включения Spectrum Analyzer
// Скопируйте эти настройки в ваш myoptions.h

// Включение Spectrum Analyzer
#define SPECTRUM_ENABLED       true    // Включить Spectrum Analyzer
#define SPECTRUM_USE_PSRAM     true    // Использовать PSRAM для FFT буферов
#define SPECTRUM_BANDS         32      // Количество полос спектра
#define SPECTRUM_FFT_SIZE      256     // Размер FFT
#define SPECTRUM_SMOOTHING     0.7f    // Сглаживание
#define SPECTRUM_PEAK_HOLD_TIME 1000.0f // Время удержания пиков (мс)
#define SPECTRUM_LOGARITHMIC   true    // Логарифмическая шкала частот
#define SPECTRUM_STEREO        true    // Стерео режим
#define SPECTRUM_REPLACE_VU    true    // Заменить VU-метр на SA

// Отключение VU-метра (если используем SA)
#define HIDE_VU                true 