/**
 * MemWatchdog — реализация. / MEM_WATCHDOG_AUTOREBOOT
 * MEM_WATCHDOG_AUTOREBOOT задаётся в myoptions.h
 */
#include "options.h"
#include "mem_watchdog.h"
#include "esp_heap_caps.h"
#include "esp_attr.h"
#include "Arduino.h"

MemWatchdog memWatchdog;

#ifdef MEM_WATCHDOG_AUTOREBOOT

static const uint32_t INT_MIN_THRESHOLD = 4096;
static const uint32_t WINDOW_MS         = 60000;
static const uint8_t  FAILS_TO_REBOOT   = 3;
static const uint32_t STABLE_RUN_MS     = 600000;  // 10 min — сброс reboot-loop

RTC_DATA_ATTR static uint32_t s_mw_boot_count = 0;

void MemWatchdog::reset() {
  m_fails = 0;
  m_first_ts = 0;
  m_reboot_armed = false;
}

void MemWatchdog::record(MWEvent /*ev*/) {
  if (m_suppressed) return;
  uint32_t now = millis();
  if (m_first_ts != 0 && (now - m_first_ts) > WINDOW_MS) {
    m_fails = 0;
    m_first_ts = 0;
  }
  if (m_first_ts == 0) m_first_ts = now;
  m_fails++;
}

MWDecision MemWatchdog::evaluate() const {
  MWDecision d = { false, 0, 0, m_fails };
  uint32_t now = millis();

  if (m_first_ts != 0 && (now - m_first_ts) > WINDOW_MS) {
    d.fails = 0;
    return d;
  }

  d.int_min  = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  d.int_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

  if (m_fails >= FAILS_TO_REBOOT && d.int_min < INT_MIN_THRESHOLD)
    d.trigger = true;

  return d;
}

void MemWatchdog::armReboot() {
  m_reboot_armed = true;
}

bool MemWatchdog::rebootArmed() const {
  return m_reboot_armed;
}

void MemWatchdog::onBoot() {
  s_mw_boot_count++;
  if (s_mw_boot_count >= 3)
    m_suppressed = true;
}

bool MemWatchdog::isSuppressed() const {
  return m_suppressed;
}

void MemWatchdog::onStableRun() {
  if (millis() > STABLE_RUN_MS)
    s_mw_boot_count = 0;
}

#else

void MemWatchdog::reset() {}
void MemWatchdog::record(MWEvent) {}
MWDecision MemWatchdog::evaluate() const { MWDecision d = {false, 0, 0, 0}; return d; }
void MemWatchdog::armReboot() {}
bool MemWatchdog::rebootArmed() const { return false; }
void MemWatchdog::onBoot() {}
bool MemWatchdog::isSuppressed() const { return false; }
void MemWatchdog::onStableRun() {}

#endif
