#ifndef MOMENT_LAYER_H
#define MOMENT_LAYER_H

#include "../ai_layer.h"

/**
 * MomentLayer - слой фразы момента
 * MomentLayer - moment phrase layer
 * 
 * Контекстный слой присутствия, не привязан напрямую к смене трека
 * Contextual presence layer, not directly tied to track change
 * 
 * MVP-0: пустая реализация (всегда молчит)
 * MVP-0: empty implementation (always silent)
 */
class MomentLayer : public AILayer {
private:
    bool _enabled;
    
public:
    MomentLayer();
    virtual ~MomentLayer() {}
    
    virtual bool process(const AIContext& context, AICandidate& out) override;
    virtual bool isEnabled() const override { return _enabled; }
    virtual void setEnabled(bool enabled) override { _enabled = enabled; }
    virtual AILayerType getType() const override { return LAYER_MOMENT; }
};

#endif // MOMENT_LAYER_H
