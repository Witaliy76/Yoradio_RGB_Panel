#ifndef MOMENT_LAYER_H
#define MOMENT_LAYER_H

/**
 * moment_layer.h - Заголовочный файл слоя момента
 * Описание: Автономный слой момента (предопределённые фразы, без LLM)
 * Автор: W76W, 4pda.to
 * Дата: 21.12.2025
 * Версия: Yoradio RGB Panel v0.9.434m-alpha
 */

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
    uint32_t _last_shown_ms;  // Время последнего показа (для интервала 60 минут) / Last show time (for 60 minute interval)
    
public:
    MomentLayer();
    virtual ~MomentLayer() {}
    
    virtual bool process(const AIContext& context, AICandidate& out) override;
    virtual bool isEnabled() const override { return _enabled; }
    virtual void setEnabled(bool enabled) override { _enabled = enabled; }
    virtual AILayerType getType() const override { return LAYER_MOMENT; }
};

#endif // MOMENT_LAYER_H
