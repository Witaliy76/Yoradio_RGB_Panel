/**
 * ai_log.h - AI Layer logging macros
 * Описание: Макросы для логирования AI Layer с поддержкой compile-time debug флага
 * Description: Logging macros for AI Layer with compile-time debug flag support
 * Автор: W76W, 4pda.to
 * Дата: 2025-01-XX
 */

#pragma once
#include <Arduino.h>
#include "../../core/options.h"  // Для доступа к myoptions.h / For access to myoptions.h

#ifndef AI_LAYER_DEBUG
  #define AI_LAYER_DEBUG 0
#endif

// Boot gate: AI_DLOG не печатается до завершения boot banner / Boot gate: AI_DLOG not printed until boot banner is complete
// По умолчанию false (boot не завершён) / Default false (boot not complete)
extern volatile bool g_ai_boot_done;

/**
 * aiLogSetBootDone - Установить флаг завершения boot / Set boot completion flag
 * aiLogSetBootDone - Set boot completion flag
 * 
 * Usage: aiLogSetBootDone(true); // После завершения boot banner / After boot banner is complete
 */
inline void aiLogSetBootDone(bool v) {
    g_ai_boot_done = v;
}

/**
 * AI_LOG - Always print (class A logs)
 * AI_LOG - Всегда печатать (логи класса A)
 * 
 * Usage: AI_LOG("[AIPlugin] Track changed, new track_id: %u", track_id);
 */
#define AI_LOG(fmt, ...)   Serial.printf((fmt "\n"), ##__VA_ARGS__)

/**
 * AI_DLOG - Print only if AI_LAYER_DEBUG == 1 AND boot is done (class B logs)
 * AI_DLOG - Печатать только если AI_LAYER_DEBUG == 1 И boot завершён (логи класса B)
 * 
 * Usage: AI_DLOG("[AIPlugin] TT score details: %s", breakdown.c_str());
 * 
 * Note: Compiles to nothing when AI_LAYER_DEBUG == 0 (no overhead)
 *       Suppressed during boot banner to avoid interleaving
 * Примечание: Компилируется в "ничего" при AI_LAYER_DEBUG == 0 (без накладных расходов)
 *             Подавлен во время boot banner чтобы избежать вклинивания
 */
#if AI_LAYER_DEBUG
  #define AI_DLOG(fmt, ...) do { if (g_ai_boot_done) Serial.printf((fmt "\n"), ##__VA_ARGS__); } while(0)
#else
  #define AI_DLOG(fmt, ...) do {} while(0)
#endif
