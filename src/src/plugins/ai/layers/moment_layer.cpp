#include "moment_layer.h"

// Предустановленные фразы для MomentLayer (без LLM)
// Predefined phrases for MomentLayer (no LLM)
static const char* kMomentPhrases[] = {
    "Всё идёт своим темпом.",
    "Сейчас можно не торопиться.",
    "Звук держит пространство ровно.",
    "Ничего не требует ответа.",
    "Тишина между событиями тоже часть дня."
};
static const size_t kMomentPhrasesCount = sizeof(kMomentPhrases) / sizeof(kMomentPhrases[0]);

MomentLayer::MomentLayer() : _enabled(true), _last_shown_ms(0) {
    // Минимальная реализация: только для пустых треков, не чаще 1 раза в 60 минут
    // Minimal implementation: only for empty tracks, not more than once per 60 minutes
}

bool MomentLayer::process(const AIContext& context, AICandidate& out) {
    // MomentLayer показывается ТОЛЬКО когда нет валидного трека
    // MomentLayer shows ONLY when there's no valid track
    if (!context.track_title.isEmpty()) {
        return false;  // Есть валидный трек - молчим / Valid track exists - silent
    }
    
    // Проверяем интервал: не чаще 1 раза в 60 минут
    // Check interval: not more than once per 60 minutes
    uint32_t now = millis();
    const uint32_t kMinIntervalMs = 60 * 60 * 1000;  // 60 минут / 60 minutes
    
    if (_last_shown_ms > 0 && (now - _last_shown_ms) < kMinIntervalMs) {
        return false;  // Еще рано / Too soon
    }
    
    // Выбираем случайную фразу из предустановленных
    // Select random phrase from predefined
    static uint32_t phrase_index = 0;
    phrase_index = (phrase_index + 1) % kMomentPhrasesCount;
    
    // Формируем кандидата
    // Build candidate
    out.text = String(kMomentPhrases[phrase_index]);
    out.source_layer = LAYER_MOMENT;
    out.min_interval_ms = kMinIntervalMs;
    out.confidence = 1.0f;  // Всегда уверены в предустановленных фразах / Always confident in predefined phrases
    
    _last_shown_ms = now;  // Фиксируем время показа / Record show time
    
    return true;  // Возвращаем кандидата / Return candidate
}
