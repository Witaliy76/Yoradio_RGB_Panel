#include "facts_layer.h"

FactsLayer::FactsLayer() : _enabled(true) {
    // MVP-0: слой всегда молчит
    // MVP-0: layer always silent
}

bool FactsLayer::process(const AIContext& context, AICandidate& out) {
    // MVP-0: пустая реализация - всегда молчим
    // MVP-0: empty implementation - always silent
    (void)context;  // Неиспользуемый параметр / Unused parameter
    (void)out;
    return false;  // Молчим / Silent
}
