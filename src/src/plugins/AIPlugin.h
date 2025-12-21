#ifndef AIPLUGIN_H
#define AIPLUGIN_H

#include "../pluginsManager/pluginsManager.h"
#include "../core/config.h"
#include "ai/ai_types.h"
#include "ai/ai_coordinator.h"
#include "ai/ai_layer.h"
#include "ai/layers/interpretation_layer.h"
#include "ai/layers/moment_layer.h"
#include "ai/providers/deepseek_provider.h"
#include "ai/ai_task.h"

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
    virtual void on_ticker() override;  // Вызывается из ticks() каждую секунду / Called from ticks() every second
    
    // Периодическая обработка результатов (вызывается из on_ticker())
    // Periodic result processing (called from on_ticker())
    void _pumpResults();

private:
    bool _initialized;
    uint32_t _current_track_id;  // ID текущего трека для защиты от stale results / Current track ID to prevent stale results
    uint32_t _last_pump_time;  // Время последнего вызова _pumpResults для rate limiting / Last _pumpResults call time for rate limiting
    bool _last_ai_activated_state;  // Последнее состояние активации AI (для логирования только при смене) / Last AI activation state (for logging only on change)
    bool _ai_decided_for_track;  // Флаг: решение принято для текущего трека (latch) / Flag: decision made for current track (latch)
    uint32_t _enqueue_at_ms;  // Время когда можно отправить LLM запрос (debounce 4 сек) / Time when LLM request can be sent (debounce 4 sec)
    uint32_t _enqueued_for_track_id;  // ID трека для которого уже отправлен запрос / Track ID for which request already enqueued
    
    // AI компоненты / AI components
    AIDisplayCoordinator _coordinator;
    InterpretationLayer _interpretationLayer;
    MomentLayer _momentLayer;
    
    // LLM провайдеры / LLM providers
    DeepSeekProvider _deepseekProvider;
    
    // AI Task Manager для асинхронного выполнения HTTPS запросов
    // AI Task Manager for asynchronous HTTPS request execution
    AITaskManager _aiTaskManager;
    
    // Формирование контекста из текущего состояния
    // Build context from current state
    void _buildContext(AIContext& context);
    
    // Парсинг artist/song из track_title
    // Parse artist/song from track_title
    void _parseTrackTitle(const String& track_title, String& artist, String& song);
    
    // Обработка кандидатов от всех слоёв
    // Process candidates from all layers
    // Returns true if any layer successfully enqueued LLM request
    // Возвращает true если любой слой успешно поставил LLM запрос в очередь
    bool _processLayers(const AIContext& context);
    
    // Проверка условий активации AI согласно runtime-манифесту
    // Check AI activation conditions according to runtime manifest
    // Returns true if ALL conditions are met: WiFi, internet, API, valid context
    // log_state_change: логировать только при смене состояния (false = тихий режим)
    // log_state_change: log only on state change (false = quiet mode)
    bool _isAIActivated(const AIContext& context, bool log_state_change = true);
    
    // Проверка готовности LLM (без требования track_title)
    // Check LLM readiness (without track_title requirement)
    // Returns true if LLM is configured and ready: WiFi, internet, provider, api_key, model
    bool _isLLMReady() const;
};

#endif // AIPLUGIN_H
