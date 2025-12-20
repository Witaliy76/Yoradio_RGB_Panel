#ifndef INTERPRETATION_LAYER_H
#define INTERPRETATION_LAYER_H

#include "../ai_layer.h"

/**
 * InterpretationLayer - слой интерпретации музыки
 * InterpretationLayer - music interpretation layer
 * 
 * Нейтральная подпись к текущему звучанию
 * Neutral signature to current sound
 * 
 * MVP-0: пустая реализация (всегда молчит)
 * MVP-0: empty implementation (always silent)
 */
class InterpretationLayer : public AILayer {
private:
    bool _enabled;
    
public:
    InterpretationLayer();
    virtual ~InterpretationLayer() {}
    
    virtual bool process(const AIContext& context, AICandidate& out) override;
    virtual bool isEnabled() const override { return _enabled; }
    virtual void setEnabled(bool enabled) override { _enabled = enabled; }
    virtual AILayerType getType() const override { return LAYER_INTERPRETATION; }
};

#endif // INTERPRETATION_LAYER_H
