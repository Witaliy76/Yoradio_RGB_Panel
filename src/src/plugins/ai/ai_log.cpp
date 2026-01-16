/**
 * ai_log.cpp - AI Layer logging implementation
 * Описание: Реализация глобального флага boot_done для AI debug-логов
 * Description: Implementation of global boot_done flag for AI debug logs
 * Автор: W76W, 4pda.to
 * Дата: 2025-01-XX
 */

#include "ai_log.h"

// Boot gate: по умолчанию false (boot не завершён) / Boot gate: default false (boot not complete)
volatile bool g_ai_boot_done = false;
