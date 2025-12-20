#ifndef AI_LAYER_H
#define AI_LAYER_H

#include "ai_types.h"

/**
 * AILayer - базовый интерфейс для AI-слоёв
 * AILayer - base interface for AI layers
 * 
 * Каждый слой реализует process() и может вернуть кандидата или молчать
 * Each layer implements process() and can return candidate or stay silent
 */
class AILayer {
public:
    virtual ~AILayer() {}
    
    /**
     * Обработка контекста и генерация кандидата (если уместно)
     * Process context and generate candidate (if appropriate)
     * 
     * @param context - контекст текущего состояния
     * @param out - выходной кандидат (заполняется если слой не молчит)
     * @return true если слой генерирует кандидата, false если молчит
     */
    virtual bool process(const AIContext& context, AICandidate& out) = 0;
    
    /**
     * Включён ли слой
     * Is layer enabled
     */
    virtual bool isEnabled() const = 0;
    
    /**
     * Включить/выключить слой
     * Enable/disable layer
     */
    virtual void setEnabled(bool enabled) = 0;
    
    /**
     * Тип слоя
     * Layer type
     */
    virtual AILayerType getType() const = 0;
};

#endif // AI_LAYER_H
