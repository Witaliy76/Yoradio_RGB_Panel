#ifndef AIPLUGIN_H
#define AIPLUGIN_H

#include "../pluginsManager/pluginsManager.h"
#include "../core/config.h"
#include "ai/ai_types.h"
#include "ai/ai_coordinator.h"
#include "ai/ai_layer.h"
#include "ai/layers/facts_layer.h"
#include "ai/layers/interpretation_layer.h"
#include "ai/layers/moment_layer.h"

/**
 * AIPlugin - плагин AI-слоя для yoRadio
 * AIPlugin - AI layer plugin for yoRadio
 * 
 * Базовый плагин для реализации AI-слоя согласно манифестам проекта
 * Base plugin for AI layer implementation according to project manifests
 */
class AIPlugin : public Plugin {
public:
    AIPlugin();
    virtual ~AIPlugin();

    // Публичный метод для явной регистрации плагина
    // Public method for explicit plugin registration
    void init();

    // Переопределяем обработчики событий / Override event handlers
    virtual void on_setup() override;
    virtual void on_track_change() override;

private:
    bool _initialized;
    
    // AI компоненты / AI components
    AIDisplayCoordinator _coordinator;
    FactsLayer _factsLayer;
    InterpretationLayer _interpretationLayer;
    MomentLayer _momentLayer;
    
    // Формирование контекста из текущего состояния
    // Build context from current state
    void _buildContext(AIContext& context);
    
    // Парсинг artist/song из track_title
    // Parse artist/song from track_title
    void _parseTrackTitle(const String& track_title, String& artist, String& song);
    
    // Обработка кандидатов от всех слоёв
    // Process candidates from all layers
    void _processLayers(const AIContext& context);
};

#endif // AIPLUGIN_H
