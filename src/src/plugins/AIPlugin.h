#ifndef AIPLUGIN_H
#define AIPLUGIN_H

/**
 * AIPlugin.h - AI Layer plugin header for yoRadio
 * Description: AI Layer plugin interface, layer management and display coordination
 * Author: W76W, 4pda.to
 * Date: 21.12.2025
 * Version: Yoradio RGB Panel v0.9.434m-r2
 */

#include "../pluginsManager/pluginsManager.h"
#include "../core/config.h"
#include "ai/ai_types.h"
#include "ai/ai_coordinator.h"
#include "ai/ai_layer.h"
#include "ai/layers/interpretation_layer.h"
#include "ai/layers/moment_layer.h"
#include "ai/providers/openai_compat_provider.h"
#include "ai/ai_task.h"

/**
 * Причины валидации track_title / Track title validation reasons
 */
enum class TrackTitleValidationReason {
    TT_VALID,                    // Валидный трек / Valid track
    TT_EMPTY,                    // Пустой track_title / Empty track_title
    TT_HARD_DENY_ERROR,          // Системная ошибка (Error connecting to...) / System error
    TT_HARD_DENY_REQUEST_FAILED, // Ошибка запроса (Request ... failed) / Request error
    TT_HARD_DENY_HASH_ERROR,     // Ошибка с ##ERROR# / Hash error
    TT_HARD_DENY_URL,            // URL в строке / URL in string
    TT_SCORE_TOO_LOW,            // Score слишком низкий (< 3) / Score too low
    TT_STATION_LIKE              // Станционная строка (score <= -2) / Station-like string
};

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
    
    // Публичный метод для уведомления об изменении состояния AI
    // Public method to notify about AI state change
    void onAiEnabledChanged(bool enabled);

private:
    bool _initialized;
    uint32_t _current_track_id;  // ID текущего трека для защиты от stale results / Current track ID to prevent stale results
    uint32_t _last_pump_time;  // Время последнего вызова _pumpResults для rate limiting / Last _pumpResults call time for rate limiting
    bool _last_ai_activated_state;  // Последнее состояние активации AI (для логирования только при смене) / Last AI activation state (for logging only on change)
    bool _ai_decided_for_track;  // Флаг: решение принято для текущего трека (latch) / Flag: decision made for current track (latch)
    uint32_t _enqueue_at_ms;  // Время когда можно отправить LLM запрос (debounce 4 сек) / Time when LLM request can be sent (debounce 4 sec)
    uint32_t _enqueued_for_track_id;  // ID трека для которого уже отправлен запрос / Track ID for which request already enqueued
    bool _ai_output_shown;  // Флаг: для текущего трека уже показан AI.FACT или AI.LISTEN / Flag: AI.FACT or AI.LISTEN already shown for current track
    uint32_t _ai_output_track_id;  // ID трека для которого был показан вывод / Track ID for which output was shown
    bool _moment_decided;  // Флаг: решение по Moment принято для текущего трека (one-shot) / Flag: Moment decision made for current track (one-shot)
    uint32_t _moment_decided_track_id;  // ID трека для которого принято решение по Moment / Track ID for which Moment decision was made
    uint32_t _ai_context_logged_track_id;  // ID трека для которого уже выведен лог невалидного контекста / Track ID for which invalid context log already printed
    uint32_t _tt_validation_logged_track_id;  // ID трека для которого уже выведен диагностический лог валидации track_title / Track ID for which track_title validation diagnostic log already printed
    TrackTitleValidationReason _last_tt_reason;  // Последняя причина валидации track_title / Last track_title validation reason
    int8_t _last_tt_score;  // Последний score валидации track_title / Last track_title validation score
    
    // AI компоненты / AI components
    AIDisplayCoordinator _coordinator;
    InterpretationLayer _interpretationLayer;
    MomentLayer _momentLayer;
    
    // LLM провайдеры / LLM providers
    OpenAICompatProvider _provider;
    
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
    
    // Проверка валидности track_title для AI (фильтрация системных/ошибочных строк)
    // Check if track_title is valid for AI (filter system/error strings)
    // Returns true if track_title is not empty and not a system/error message
    // Устанавливает _last_tt_reason и _last_tt_score / Sets _last_tt_reason and _last_tt_score
    bool _isValidTrackTitleForAI(const String& t);
    
    // Helper для логов один раз на track_id / Helper for logs once per track_id
    bool _logOncePerTrack(uint32_t track_id, const char* message);
    
    // Helper: диагностическое логирование валидации track_title / Helper: diagnostic logging of track_title validation
    void _logTrackTitleValidation(uint32_t track_id, const String& title, bool is_valid);
};

#endif // AIPLUGIN_H
