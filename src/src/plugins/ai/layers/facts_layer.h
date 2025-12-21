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
 * Runtime Manifest Section 4.1:
 * - Формат: "Артист — Трек. Альбом: <Название>, <Год>"
 * - Альбом и год обязательны
 * - Только проверенные данные
 * - Если нет надёжных данных → Facts не выводится
 * 
 * MVP-1: Структура готова, но молчит без API данных
 * MVP-1: Structure ready, but silent without API data
 */
class FactsLayer : public AILayer {
private:
    bool _enabled;
    
    // MVP-1: Заглушка для будущего API
    // MVP-1: Stub for future API
    bool _hasAPIData(const AIContext& context) const;
    
public:
    FactsLayer();
    virtual ~FactsLayer() {}
    
    virtual bool process(const AIContext& context, AICandidate& out) override;
    virtual bool isEnabled() const override { return _enabled; }
    virtual void setEnabled(bool enabled) override { _enabled = enabled; }
    virtual AILayerType getType() const override { return LAYER_FACTS; }
};

#endif // FACTS_LAYER_H
