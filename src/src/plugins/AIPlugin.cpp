/**
 * AIPlugin.cpp - Реализация плагина AI-слоя для yoRadio
 * Описание: Основная логика AI-слоя, обработка событий, координация слоёв
 * Автор: W76W, 4pda.to
 * Дата: 21.12.2025
 * Версия: Yoradio RGB Panel v0.9.434m-alpha
 */

#include "AIPlugin.h"
#include "ai/utils/utf8_casefold_search.h"
#include "../core/network.h"
#include "../core/player.h"
#include "../core/display.h"
#include <WiFi.h>  // Для проверки WiFi статуса / For WiFi status check

extern Config config;
extern MyNetwork network;
extern Player player;

AIPlugin::AIPlugin() : _initialized(false) {
    // Не регистрируем плагин в конструкторе - регистрация будет явной в setup()
    // Don't register plugin in constructor - registration will be explicit in setup()
}

AIPlugin::~AIPlugin() {
    // Деструктор для очистки ресурсов (если понадобится)
    // Destructor for resource cleanup (if needed)
}

void AIPlugin::init() {
    // Явная регистрация плагина
    // Explicit plugin registration
    registerPlugin();
}

void AIPlugin::on_setup() {
    Serial.println("[AIPlugin] on_setup() called - AI plugin initialized");
    Serial.println("[AIPlugin] MVP-2: Architecture ready with LLM provider integration");
    Serial.println("[AIPlugin] Runtime Manifest: AI is optional, silence is valid");
    
    // Инициализация AI Task Manager для асинхронного выполнения HTTPS запросов
    // Initialize AI Task Manager for asynchronous HTTPS request execution
    if (!_aiTaskManager.begin(&_deepseekProvider)) {
        Serial.println("[AIPlugin] WARNING: AI Task Manager initialization failed");
    }
    
    // Передаём Task Manager в InterpretationLayer
    // Pass Task Manager to InterpretationLayer
    _interpretationLayer.setTaskManager(&_aiTaskManager);
    
    _last_pump_time = 0;  // Инициализация времени последнего pump / Initialize last pump time
    _last_ai_activated_state = false;  // Инициализация состояния активации AI / Initialize AI activation state
    _ai_decided_for_track = false;  // Инициализация флага принятия решения / Initialize decision flag
    _enqueue_at_ms = 0;  // Инициализация времени debounce / Initialize debounce time
    _enqueued_for_track_id = 0;  // Инициализация ID трека для enqueue / Initialize track ID for enqueue
    _initialized = true;
}

void AIPlugin::_parseTrackTitle(const String& track_title, String& artist, String& song) {
    artist = "";
    song = "";
    
    if (track_title.isEmpty()) {
        return;
    }
    
    // Ищем разделитель " - " / Look for separator " - "
    int sep_pos = track_title.indexOf(" - ");
    if (sep_pos > 0) {
        artist = track_title.substring(0, sep_pos);
        artist.trim();
        song = track_title.substring(sep_pos + 3);
        song.trim();
    } else {
        // Нет разделителя - весь текст в song
        // No separator - all text goes to song
        song = track_title;
        song.trim();
    }
}

void AIPlugin::_buildContext(AIContext& context) {
    // Музыка/радио / Music/radio
    context.station_name = String(config.station.name);
    context.is_playing = player.isRunning();
    
    // Фильтруем track_title от служебных строк
    // Filter track_title from service strings
    String raw_title = String(config.station.title);
    context.track_title = AIDisplayCoordinator::filterTrackTitle(raw_title);
    
    // Парсим artist/song из track_title
    // Parse artist/song from track_title
    _parseTrackTitle(context.track_title, context.artist, context.song);
    
    // Время / Time
    // Проверяем валидность времени / Check time validity
    if (network.timeinfo.tm_year > 100) {
        context.current_hour = network.timeinfo.tm_hour;  // 0-23
    } else {
        context.current_hour = 255;  // Время невалидно / Time invalid
    }
    
    context.uptime_ms = millis();
}

void AIPlugin::_pumpResults() {
    // Периодическая обработка результатов AI Task (независимо от смены трека)
    // Periodic processing of AI Task results (independent of track change)
    uint32_t current_time = millis();
    
    // Rate limiting: вызываем не чаще чем каждые 300ms для снижения нагрузки
    // Rate limiting: call no more than every 300ms to reduce load
    if (current_time - _last_pump_time < 300) {
        return;  // Слишком рано / Too soon
    }
    _last_pump_time = current_time;
    
    // Latch: если решение уже принято для текущего трека - игнорируем новые результаты
    // Latch: if decision already made for current track - ignore new results
    if (_ai_decided_for_track) {
        return;  // Решение принято, больше ничего не меняем / Decision made, don't change anything
    }
    
    AIRequestResult result;
    while (_aiTaskManager.getResult(result)) {
        // Диагностический лог: результат получен из очереди (показываем raw данные)
        // Diagnostic log: result dequeued (show raw data)
        Serial.printf("[AIPlugin] Dequeued result: ok=%d mode=%s track_id=%u current=%u conf=%.2f\n",
                      result.ok, result.mode, result.track_id, _current_track_id, result.confidence);
        
        // Проверяем, не устарел ли результат / Check if result is stale
        if (result.track_id != _current_track_id) {
            Serial.printf("[AIPlugin] Stale result dropped: result_id=%u current_id=%u\n", 
                          result.track_id, _current_track_id);
            continue;  // Пропускаем устаревший результат / Skip stale result
        }
        
        // Latch: если решение уже принято для этого трека - игнорируем результат
        // Latch: if decision already made for this track - ignore result
        if (_ai_decided_for_track) {
            Serial.println("[AIPlugin] Decision already made for this track, ignoring result");
            continue;
        }
        
        if (!result.ok) {
            Serial.println("[AIPlugin] Coordinator reject reason: not_ok");
            // ok=false - молчим, решение принято (silence is valid)
            // ok=false - silence, decision made (silence is valid)
            _ai_decided_for_track = true;
            continue;
        }
        
        if (strlen(result.text) == 0) {
            Serial.println("[AIPlugin] Coordinator reject reason: empty");
            // Пустой текст - молчим, решение принято (silence is valid)
            // Empty text - silence, decision made (silence is valid)
            _ai_decided_for_track = true;
            continue;
        }
        
        // Предохранитель "строгих фактов" по манифесту с risk scoring
        // "Strict facts" safety gate per manifest with risk scoring
        String mode = String(result.mode);
        float confidence = result.confidence;
        bool was_downgraded = false;  // Флаг: был ли downgrade fact → listen / Flag: was there a downgrade fact → listen
        
        if (mode == "fact") {
            // Risk-scoring для безопасности фактов (без String, без heap-аллокаций)
            // We do NOT hard-ban phrases; we raise required confidence gradually.
            // Goal: keep interesting facts, but suppress high-risk narrative claims unless confidence is high.
            
            // Helper lambda для clamp
            auto clampf = [](float v, float lo, float hi) -> float {
                if (v < lo) return lo;
                if (v > hi) return hi;
                return v;
            };
            
            // Используем const char* напрямую, без String
            const char* txt = result.text;
            
            // Class A: credits / production / collaborations (very specific, high hallucination cost)
            static const char* kRiskA[] = {
                "соавтор", "соавторстве", "продюсер", "продюсировал", "продюсирован",
                "работал с", "в соавторстве",
                "collaborat", "producer", "produced", "co-wrote", "cowrote", "co-writer", "co writer"
            };
            
            // Class B: narrative / origin / dedication / charity / soundtrack (risky but common in true facts)
            static const char* kRiskB1[] = { // +1
                "впервые", "originally", "изначально", "первоначально"
            };
            static const char* kRiskB2[] = { // +2
                "благотвор", "благотворительн", "charity", "benefit",
                "посвящен", "посвящена", "в честь", "в память", "dedicated", "in honor", "in honour",
                "написана для", "записана для", "создана для", "written for",
                "soundtrack", "саундтрек", "по заказу", "commissioned"
            };
            static const char* kRiskB3[] = { // +2 (abbrev expansions are often hallucinated)
                "расшифровывается как"
            };
            
            int riskScore = 0;
            bool hitA = utf8_contains_any_ci(txt, kRiskA, sizeof(kRiskA)/sizeof(kRiskA[0]));
            if (hitA) riskScore += 3;
            
            bool hitB1 = utf8_contains_any_ci(txt, kRiskB1, sizeof(kRiskB1)/sizeof(kRiskB1[0]));
            if (hitB1) riskScore += 1;
            
            bool hitB2 = utf8_contains_any_ci(txt, kRiskB2, sizeof(kRiskB2)/sizeof(kRiskB2[0]));
            if (hitB2) riskScore += 2;
            
            bool hitB3 = utf8_contains_any_ci(txt, kRiskB3, sizeof(kRiskB3)/sizeof(kRiskB3[0]));
            if (hitB3) riskScore += 2;
            
            // Dynamic confidence threshold for facts
            // Base 0.85, then increases with riskScore
            float required_conf = 0.85f;
            if (riskScore == 0) {
                required_conf = 0.85f;
            } else if (riskScore <= 2) {
                required_conf = 0.90f;
            } else if (riskScore <= 4) {
                required_conf = 0.93f;
            } else {
                required_conf = 0.95f;
            }
            
            // If Class A is hit, never go below 0.95 (keep old behavior / improve clarity)
            if (hitA && required_conf < 0.95f) {
                required_conf = 0.95f;
            }
            
            // Cap at 0.97 if you want ultra strict at very high risk
            required_conf = clampf(required_conf, 0.85f, 0.97f);
            
            // Apply downgrade only for "fact"
            if (confidence < required_conf) {
                // Логируем с куском текста для диагностики
                Serial.printf("[AIPlugin] RiskScore=%d (A=%d B1=%d B2=%d B3=%d) required_conf=%.2f, got=%.2f -> downgrade fact->listen text=\"%.100s\"\n",
                              riskScore, hitA?1:0, hitB1?1:0, hitB2?1:0, hitB3?1:0, required_conf, confidence, result.text);
                mode = "listen";
                confidence = 0.5f;
                was_downgraded = true;
            } else {
                Serial.printf("[AIPlugin] RiskScore=%d required_conf=%.2f, got=%.2f -> fact allowed\n",
                              riskScore, required_conf, confidence);
            }
        }
        
        // Расширенный диагностический лог с effective_mode и was_downgraded
        // Extended diagnostic log with effective_mode and was_downgraded
        Serial.printf("[AIPlugin] Processed result: original_mode=%s effective_mode=%s was_downgraded=%d\n",
                      result.mode, mode.c_str(), was_downgraded ? 1 : 0);
        
        // Если был downgrade fact → listen: не показываем (silence is valid по манифесту)
        // If there was downgrade fact → listen: don't show (silence is valid per manifest)
        if (was_downgraded) {
            // Не показываем downgraded listen, чтобы не спамить одинаковой строкой
            // Don't show downgraded listen to avoid spamming the same line
            // MVP-1: Очищаем виджет при downgrade / MVP-1: Clear widget on downgrade
            display.setAIInterpretation("");
            Serial.println("[AIPlugin] Coordinator reject reason: downgraded_fact_to_listen (silence is valid)");
            // Решение принято: молчим / Decision made: silence
            _ai_decided_for_track = true;
            continue;  // Пропускаем этот результат / Skip this result
        }
        
        // Если mode="listen" пришел напрямую от LLM — используем оригинальный текст
        // If mode="listen" came directly from LLM — use original text
        // Формируем кандидата из результата / Build candidate from result
        AICandidate candidate;
        candidate.text = String(result.text);
        // Разводим source_layer по mode / Set source_layer based on mode
        if (mode == "fact") {
            candidate.source_layer = LAYER_FACTS;
        } else {
            candidate.source_layer = LAYER_INTERPRETATION;  // mode == "listen"
        }
        candidate.min_interval_ms = 10000;
        candidate.confidence = confidence;  // Используем скорректированную уверенность / Use adjusted confidence
        
        // Coordinator решает: показывать ли / Coordinator decides: show or not
        bool should_show = _coordinator.shouldShow(&candidate, current_time);
        
        if (should_show) {
            _coordinator.markAsShown(&candidate, current_time);
            
            // MVP-1: Вывод интерпретации на экран / MVP-1: Display interpretation on screen
            display.setAIInterpretation(candidate.text);
            
            // Логируем с корректным префиксом в зависимости от mode / Log with correct prefix based on mode
            if (mode == "fact") {
                Serial.print("##AI.FACT#: ");
            } else {
                Serial.print("##AI.LISTEN#: ");  // mode == "listen"
            }
            Serial.println(candidate.text);
            Serial.println("[AIPlugin] Coordinator: show");
            // Решение принято: текст показан / Decision made: text shown
            _ai_decided_for_track = true;
        } else {
            // Coordinator отклонил - логируем причину (будет видно в shouldShow если добавим детализацию)
            // Coordinator rejected - log reason (will be visible in shouldShow if we add details)
            Serial.println("[AIPlugin] Coordinator reject reason: rate_limit_or_duplicate");
            // НЕ выставляем latch здесь - это может быть промежуточный результат
            // Если это финальный результат для трека, latch выставится при следующем невалидном результате
            // DON'T set latch here - this may be intermediate result
            // If this is final result for track, latch will be set on next invalid result
        }
    }
}

bool AIPlugin::_processLayers(const AIContext& context) {
    Serial.println("[AIPlugin] _processLayers() entered");
    uint32_t current_time = millis();
    
    // Обрабатываем результаты (также вызывается периодически через _pumpResults)
    // Process results (also called periodically via _pumpResults)
    _pumpResults();
    
    // Latch: если решение уже принято для текущего трека - не обрабатываем слои
    // Latch: if decision already made for current track - don't process layers
    if (_ai_decided_for_track) {
        Serial.println("[AIPlugin] Decision already made for track, skipping layer processing");
        return false;
    }
    
    // Обновляем track_id в InterpretationLayer перед обработкой
    // Update track_id in InterpretationLayer before processing
    _interpretationLayer.setTrackId(_current_track_id);
    
    AICandidate candidate;
    bool enqueued_any = false;  // Флаг успешного enqueue / Flag for successful enqueue
    
    // Обрабатываем слой Interpretation (MomentLayer обрабатывается только в тикере)
    // Process Interpretation layer (MomentLayer processed only in ticker)
    AILayer* layers[] = { &_interpretationLayer };
    const char* layer_names[] = { "Interpretation" };
    
    Serial.print("[AIPlugin] Processing ");
    Serial.print(1);
    Serial.println(" layer");
    
    for (size_t i = 0; i < 1; i++) {
        Serial.print("[AIPlugin] Checking layer: ");
        Serial.println(layer_names[i]);
        
        if (!layers[i]->isEnabled()) {
            Serial.print("[AIPlugin] Layer ");
            Serial.print(layer_names[i]);
            Serial.println(" disabled, skipping");
            continue;  // Слой выключен / Layer disabled
        }
        
        Serial.print("[AIPlugin] Calling layer->process() for ");
        Serial.println(layer_names[i]);
        
        // Слой обрабатывает контекст / Layer processes context
        if (layers[i]->process(context, candidate)) {
            // InterpretationLayer возвращает true только при успешном enqueueRequest()
            // InterpretationLayer returns true only on successful enqueueRequest()
            enqueued_any = true;
            Serial.print("[AIPlugin] Layer ");
            Serial.print(layer_names[i]);
            Serial.println(" enqueued LLM request");
            
            // Coordinator решает: показывать ли / Coordinator decides: show or not
            if (_coordinator.shouldShow(&candidate, current_time)) {
                // Отмечаем как показанное / Mark as shown
                _coordinator.markAsShown(&candidate, current_time);
                
                // Логирование происходит в слоях (например, ##AI.INTERP# в InterpretationLayer)
                // Logging happens in layers (e.g., ##AI.INTERP# in InterpretationLayer)
                
                // TODO: В будущем здесь будет вывод на экран
                // TODO: In the future, screen output will be here
            }
            // Не логируем фильтрацию или молчание для уменьшения шума
            // Don't log filtering or silence to reduce noise
        }
        // Слой молчит / Layer silent - не логируем (Runtime Manifest: минимизация логов)
        // Layer silent - don't log (Runtime Manifest: minimize logs)
    }
    
    return enqueued_any;
}

bool AIPlugin::_isAIActivated(const AIContext& context, bool log_state_change) {
    // Runtime Manifest Section 1: AI activation conditions
    // Все условия должны выполняться одновременно / All conditions must be met simultaneously
    
    // 0. AI включён в настройках / AI enabled in settings
    if (!config.store.ai_enabled) {
        if (log_state_change) {
            Serial.println("[AIPlugin] _isAIActivated: ai_enabled=false");
        }
        return false;
    }
    
    // 1. Wi‑Fi подключён / Wi‑Fi connected
    if (network.status != CONNECTED || WiFi.status() != WL_CONNECTED) {
        if (log_state_change) {
            Serial.println("[AIPlugin] _isAIActivated: WiFi not connected");
        }
        return false;
    }
    
    // 2. Интернет доступен (проверяем наличие IP адреса) / Internet available (check IP)
    IPAddress ip = WiFi.localIP();
    if (ip == IPAddress(0, 0, 0, 0)) {
        if (log_state_change) {
            Serial.println("[AIPlugin] _isAIActivated: No IP address");
        }
        return false;
    }
    
    // 3. Провайдер LLM настроен / LLM provider configured
    if (config.store.llm_provider == LLM_NONE) {
        if (log_state_change) {
            Serial.println("[AIPlugin] _isAIActivated: llm_provider=LLM_NONE");
        }
        return false;
    }
    
    // 4. API ключ и модель настроены / API key and model configured
    String api_key = String(config.store.ai_api_key);
    String model = String(config.store.ai_model);
    if (api_key.isEmpty() || model.isEmpty()) {
        if (log_state_change) {
            Serial.println("[AIPlugin] _isAIActivated: API key or model empty");
        }
        return false;
    }
    
    // 5. Валидный музыкальный контекст (реальный трек, не системный статус)
    // Valid music context (real track, not system status)
    // Ослабляем требование: достаточно track_title, artist/song могут быть пустыми
    // Relaxed requirement: track_title is enough, artist/song may be empty
    if (context.track_title.isEmpty()) {
        if (log_state_change) {
            Serial.print("[AIPlugin] _isAIActivated: Invalid context - track_title empty");
            Serial.println();
        }
        return false;
    }
    
    // Все условия выполнены / All conditions met
    if (log_state_change) {
        Serial.println("[AIPlugin] _isAIActivated: All conditions met");
    }
    return true;
}

bool AIPlugin::_isLLMReady() const {
    // Проверка готовности LLM без требования track_title
    // Check LLM readiness without track_title requirement
    // Используется для enqueue после debounce в тикере
    // Used for enqueue after debounce in ticker
    
    // 0. AI включён в настройках / AI enabled in settings
    if (!config.store.ai_enabled) {
        return false;
    }
    
    // 1. Wi‑Fi подключён / Wi‑Fi connected
    if (network.status != CONNECTED || WiFi.status() != WL_CONNECTED) {
        return false;
    }
    
    // 2. Интернет доступен (проверяем наличие IP адреса) / Internet available (check IP)
    IPAddress ip = WiFi.localIP();
    if (ip == IPAddress(0, 0, 0, 0)) {
        return false;
    }
    
    // 3. Провайдер LLM настроен / LLM provider configured
    if (config.store.llm_provider == LLM_NONE) {
        return false;
    }
    
    // 4. API ключ и модель настроены / API key and model configured
    String api_key = String(config.store.ai_api_key);
    String model = String(config.store.ai_model);
    if (api_key.isEmpty() || model.isEmpty()) {
        return false;
    }
    
    // Все условия выполнены (без требования track_title) / All conditions met (without track_title requirement)
    return true;
}

void AIPlugin::on_track_change() {
    // Инкрементируем ID трека при валидной смене трека
    // Increment track ID on valid track change
    _current_track_id++;
    Serial.printf("[AIPlugin] Track changed, new track_id: %u\n", _current_track_id);
    
    // Сбрасываем флаг принятия решения для нового трека
    // Reset decision flag for new track
    _ai_decided_for_track = false;
    
    // Устанавливаем время debounce: запрос можно отправить через 4 секунды
    // Set debounce time: request can be sent after 4 seconds
    uint32_t now = millis();
    _enqueue_at_ms = now + 4000;  // Debounce 4 секунды / Debounce 4 seconds
    _enqueued_for_track_id = 0;  // Сбрасываем флаг отправки запроса / Reset enqueue flag
    
    // MVP-1: Очищаем виджет интерпретации при смене трека / MVP-1: Clear interpretation widget on track change
    display.setAIInterpretation("");
    
    if (!_initialized) {
        Serial.println("[AIPlugin] on_track_change() called but not initialized");
        return;
    }

    // MVP-2: Формируем контекст и проверяем условия активации
    // MVP-2: Build context and check activation conditions
    AIContext context;
    _buildContext(context);
    
    // Временное логирование для отладки / Temporary logging for debugging
    Serial.print("[AIPlugin] on_track_change() - ai_enabled=");
    Serial.print(config.store.ai_enabled);
    Serial.print(", llm_provider=");
    Serial.print(config.store.llm_provider);
    Serial.print(", has_api_key=");
    Serial.print(strlen(config.store.ai_api_key) > 0);
    Serial.print(", track_title=\"");
    Serial.print(context.track_title);
    Serial.println("\"");
    
    // Runtime Manifest: AI активируется только при выполнении всех условий
    // Runtime Manifest: AI activates only when all conditions are met
    if (!_isAIActivated(context)) {
        Serial.println("[AIPlugin] AI not activated - skipping");
        return;
    }
    
    Serial.println("[AIPlugin] AI activated - debounce scheduled, will process layers after 4s");
    // НЕ вызываем _processLayers() сразу - запрос уйдет через тикер после debounce
    // DON'T call _processLayers() immediately - request will be sent via ticker after debounce
}

void AIPlugin::on_ticker() {
    // Вызывается из ticks() каждую секунду / Called from ticks() every second
    // Вызываем _pumpResults() для периодической обработки результатов AI Task
    // Call _pumpResults() for periodic processing of AI Task results
    _pumpResults();
    
    uint32_t now = millis();
    AIContext context;
    _buildContext(context);
    
    // Проверяем, активирован ли AI (логируем только при смене состояния)
    // Check if AI is activated (log only on state change)
    bool ai_activated = _isAIActivated(context, false);  // Не логируем каждый тик / Don't log every tick
    if (ai_activated != _last_ai_activated_state) {
        _last_ai_activated_state = ai_activated;
        _isAIActivated(context, true);  // Логируем смену состояния / Log state change
    }
    
    // ИСКЛЮЧЕНИЕ: отправка LLM запроса после debounce (единственный случай enqueue из тикера)
    // EXCEPTION: send LLM request after debounce (only case of enqueue from ticker)
    // Используем _isLLMReady() вместо _isAIActivated() - не требует track_title
    // Use _isLLMReady() instead of _isAIActivated() - doesn't require track_title
    bool llm_ready = _isLLMReady();
    if (llm_ready && _enqueue_at_ms > 0 && now >= _enqueue_at_ms && 
        _enqueued_for_track_id != _current_track_id && !_ai_decided_for_track) {
        // Диагностический лог перед enqueue / Diagnostic log before enqueue
        Serial.printf("[AIPlugin] Debounce check: track_title_len=%d llm_ready=%d ai_activated=%d\n",
                      context.track_title.length(), llm_ready ? 1 : 0, ai_activated ? 1 : 0);
        
        // Debounce прошел, отправляем запрос один раз для текущего трека
        // Debounce passed, send request once for current track
        Serial.println("[AIPlugin] Debounce passed, processing layers to enqueue LLM request");
        bool enqueued = _processLayers(context);
        
        // ВСЕГДА выставляем флаги после попытки (строго 1 attempt per track)
        // ALWAYS set flags after attempt (strictly 1 attempt per track)
        _enqueued_for_track_id = _current_track_id;  // Помечаем что попытка была / Mark attempt as made
        _enqueue_at_ms = 0;  // Сбрасываем debounce таймер / Reset debounce timer
        
        if (enqueued) {
            Serial.println("[AIPlugin] LLM request enqueued successfully");
        } else {
            Serial.println("[AIPlugin] LLM enqueue attempt failed (rate limit/busy) -> silence for this track");
        }
    }
    
        // MomentLayer: обрабатываем только если нет валидного трека
        // MomentLayer: process only if no valid track
    // MomentLayer автономен: НЕ зависит от LLM (llm_provider/api_key/model)
    // MomentLayer is autonomous: does NOT depend on LLM (llm_provider/api_key/model)
    // Latch: если решение уже принято для трека - MomentLayer не должен вытеснять текст
    // Latch: if decision already made for track - MomentLayer should not replace text
    if (context.track_title.isEmpty() && !_ai_decided_for_track) {
        // Проверяем минимальные условия для MomentLayer (без LLM зависимостей)
        // Check minimal conditions for MomentLayer (without LLM dependencies)
        bool moment_ready = config.store.ai_enabled &&
                            (network.status == CONNECTED) &&
                            (WiFi.status() == WL_CONNECTED) &&
                            (WiFi.localIP() != IPAddress(0, 0, 0, 0));
        
        if (moment_ready) {
            // MomentLayer автономен: только локальные шаблоны, никаких LLM запросов
            // MomentLayer is autonomous: only local templates, no LLM requests
            AICandidate moment_candidate;
            if (_momentLayer.process(context, moment_candidate)) {
                // MomentLayer вернул кандидата - обрабатываем его
                // MomentLayer returned candidate - process it
                uint32_t current_time = millis();
                if (_coordinator.shouldShow(&moment_candidate, current_time)) {
                    _coordinator.markAsShown(&moment_candidate, current_time);
                    display.setAIInterpretation(moment_candidate.text);
                    Serial.print("##AI.MOMENT#: ");
                    Serial.println(moment_candidate.text);
                }
            }
        }
    }
}

// Публичный метод для периодического вызова (можно вызывать из main loop)
// Public method for periodic calls (can be called from main loop)
// ВАЖНО: Для интеграции в main loop нужно добавить вызов в network.ticks() или main loop()
// IMPORTANT: To integrate into main loop, add call to network.ticks() or main loop()
// Пока вызывается только из _processLayers() при смене трека
// Currently called only from _processLayers() on track change
