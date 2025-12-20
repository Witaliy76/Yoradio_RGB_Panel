#include "interpretation_layer.h"

InterpretationLayer::InterpretationLayer() : _enabled(true) {
    // MVP-0: слой всегда молчит
    // MVP-0: layer always silent
}

bool InterpretationLayer::process(const AIContext& context, AICandidate& out) {
    // MVP-0: пустая реализация - всегда молчим
    // MVP-0: empty implementation - always silent
    (void)context;  // Неиспользуемый параметр / Unused parameter
    (void)out;
    return false;  // Молчим / Silent
}
