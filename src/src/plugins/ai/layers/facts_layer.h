#ifndef FACTS_LAYER_H
#define FACTS_LAYER_H

#include "../ai_layer.h"

/**
 * FactsLayer - слой фактов о музыке
 * FactsLayer - music facts layer
 * 
 * Информационный слой о треке или станции
 * Informational layer about track or station
 * 
 * MVP-0: пустая реализация (всегда молчит)
 * MVP-0: empty implementation (always silent)
 */
class FactsLayer : public AILayer {
private:
    bool _enabled;
    
public:
    FactsLayer();
    virtual ~FactsLayer() {}
    
    virtual bool process(const AIContext& context, AICandidate& out) override;
    virtual bool isEnabled() const override { return _enabled; }
    virtual void setEnabled(bool enabled) override { _enabled = enabled; }
    virtual AILayerType getType() const override { return LAYER_FACTS; }
};

#endif // FACTS_LAYER_H
