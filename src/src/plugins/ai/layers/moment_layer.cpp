#include "moment_layer.h"

MomentLayer::MomentLayer() : _enabled(true) {
    // MVP-0: слой всегда молчит
    // MVP-0: layer always silent
}

bool MomentLayer::process(const AIContext& context, AICandidate& out) {
    // MVP-0: пустая реализация - всегда молчим
    // MVP-0: empty implementation - always silent
    (void)context;  // Неиспользуемый параметр / Unused parameter
    (void)out;
    return false;  // Молчим / Silent
}
