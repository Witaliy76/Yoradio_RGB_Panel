#include "facts_layer.h"

FactsLayer::FactsLayer() : _enabled(true) {
    // MVP-1: структура готова для будущего API
    // MVP-1: structure ready for future API
}

bool FactsLayer::_hasAPIData(const AIContext& context) const {
    // MVP-1: Заглушка - API ещё не реализовано
    // MVP-1: Stub - API not yet implemented
    // В будущем здесь будет проверка наличия данных от внешнего API
    // In the future, this will check for data from external API
    (void)context;
    return false;
}

bool FactsLayer::process(const AIContext& context, AICandidate& out) {
    if (!_enabled) {
        return false;  // Слой выключен / Layer disabled
    }
    
    // Runtime Manifest Section 4.1: только проверенные данные
    // Runtime Manifest Section 4.1: only verified data
    if (!_hasAPIData(context)) {
        // MVP-1: Нет данных от API → молчим
        // MVP-1: No API data → silent
        return false;
    }
    
    // TODO: MVP-2+ Реализация с внешним API
    // TODO: MVP-2+ Implementation with external API
    // Формат по манифесту: "Артист — Трек. Альбом: <Название>, <Год>"
    // Manifest format: "Artist — Track. Album: <Name>, <Year>"
    
    // Пока всегда молчим (нет API) / Always silent for now (no API)
    (void)out;
    return false;
}
