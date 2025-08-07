#include "esp32_perfmon.h"
#include <Arduino.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "sdkconfig.h"

#include "esp_log.h"
static const char *TAG = "perfmon";

static uint64_t idle0Calls = 0;
static uint64_t idle1Calls = 0;
static uint32_t lastCpu0 = 0;
static uint32_t lastCpu1 = 0;
static uint32_t lastUpdate = 0;

// Буфер для сглаживания значений
#define SMOOTH_BUFFER_SIZE 5
static uint32_t cpu0Buffer[SMOOTH_BUFFER_SIZE] = {0};
static uint32_t cpu1Buffer[SMOOTH_BUFFER_SIZE] = {0};
static uint8_t bufferIndex = 0;

// Определяем MaxIdleCalls в зависимости от частоты CPU
#if defined(CONFIG_ESP32_DEFAULT_CPU_FREQ_240)
static const uint64_t MaxIdleCalls = 1855000;
#elif defined(CONFIG_ESP32_DEFAULT_CPU_FREQ_160)
static const uint64_t MaxIdleCalls = 1233100;
#elif defined(CONFIG_ESP32S3_DEFAULT_CPU_FREQ_240)
static const uint64_t MaxIdleCalls = 1855000;
#elif defined(CONFIG_ESP32S3_DEFAULT_CPU_FREQ_160)
static const uint64_t MaxIdleCalls = 1233100;
#else
// Для других частот используем значение по умолчанию
static const uint64_t MaxIdleCalls = 1855000;
#endif

static bool idle_task_0()
{
	idle0Calls += 1;
	return false;
}

static bool idle_task_1()
{
	idle1Calls += 1;
	return false;
}

// Функция для вычисления среднего значения из буфера
static uint32_t calculateAverage(uint32_t* buffer) {
    uint32_t sum = 0;
    for(int i = 0; i < SMOOTH_BUFFER_SIZE; i++) {
        sum += buffer[i];
    }
    return sum / SMOOTH_BUFFER_SIZE;
}

uint32_t perfmon_get_cpu_usage(uint8_t core) {
    uint32_t currentMillis = millis();
    
    // Обновляем значения каждую секунду
    if (currentMillis - lastUpdate >= 1000) {
        float idle0 = idle0Calls;
        float idle1 = idle1Calls;
        
        // Отладочный вывод счетчиков
        //printf("[CPU] Idle calls - Core 0: %llu, Core 1: %llu\n", idle0Calls, idle1Calls);
        
        // Сбрасываем счетчики
        idle0Calls = 0;
        idle1Calls = 0;
        
        // Вычисляем загрузку CPU с учетом приоритетов задач
        uint32_t cpu0, cpu1;
        
        // Для ядра 0 (аудио)
        if (idle0 > MaxIdleCalls) {
            cpu0 = 0; // Если idle задача выполняется больше ожидаемого, CPU свободен
        } else {
            cpu0 = 100 - (idle0 * 100 / MaxIdleCalls);
        }
        
        // Для ядра 1 (основной код)
        if (idle1 > MaxIdleCalls) {
            cpu1 = 0;
        } else {
            cpu1 = 100 - (idle1 * 100 / MaxIdleCalls);
        }
        
        // Ограничиваем значения
        if (cpu0 > 100) cpu0 = 100;
        if (cpu1 > 100) cpu1 = 100;
        
        // Добавляем значения в буфер
        cpu0Buffer[bufferIndex] = cpu0;
        cpu1Buffer[bufferIndex] = cpu1;
        bufferIndex = (bufferIndex + 1) % SMOOTH_BUFFER_SIZE;
        
        // Вычисляем средние значения
        lastCpu0 = calculateAverage(cpu0Buffer);
        lastCpu1 = calculateAverage(cpu1Buffer);
        
        lastUpdate = currentMillis;
        
        // Отладочный вывод расчетов
        //printf("[CPU] Raw values - Core 0: %d%%, Core 1: %d%%\n", cpu0, cpu1);
        //printf("[CPU] Smoothed - Core 0: %d%%, Core 1: %d%%, Avg: %d%%\n", 
        //       lastCpu0, lastCpu1, (lastCpu0 + lastCpu1) / 2);
    }
    
    // Возвращаем среднее значение загрузки CPU
    return (lastCpu0 + lastCpu1) / 2;
}

esp_err_t perfmon_start()
{
	ESP_ERROR_CHECK(esp_register_freertos_idle_hook_for_cpu(idle_task_0, 0));
	ESP_ERROR_CHECK(esp_register_freertos_idle_hook_for_cpu(idle_task_1, 1));
	//printf("[CPU] Мониторинг CPU запущен (MaxIdleCalls: %llu)\n", MaxIdleCalls);
	return ESP_OK;
}
