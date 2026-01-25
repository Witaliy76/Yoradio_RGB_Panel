/**
 * MemWatchdog — fail-safe автоперезапуск при деградации внутренней RAM (TLS/фрагментация).
 * MEM_WATCHDOG_AUTOREBOOT: включение в myoptions.h (отключить: закомментировать define).
 */
#ifndef MEM_WATCHDOG_H
#define MEM_WATCHDOG_H

#include <stdint.h>
#include <stdbool.h>

enum class MWEvent : uint8_t {
  TLS_FAIL,
  HTTP_FAIL,
  CONN_LOST,
};

struct MWDecision {
  bool     trigger;
  uint32_t int_min;
  uint32_t int_free;
  uint8_t  fails;
};

class MemWatchdog {
public:
  void reset();
  void record(MWEvent ev);
  MWDecision evaluate() const;

  void armReboot();
  bool rebootArmed() const;

  void onBoot();
  bool isSuppressed() const;

  /** Вызывать из main loop при millis() > 600000: сброс счётчика reboot-loop. */
  void onStableRun();

private:
  uint8_t  m_fails       = 0;
  uint32_t m_first_ts    = 0;
  bool     m_reboot_armed = false;
  bool     m_suppressed   = false;
};

extern MemWatchdog memWatchdog;

#endif
