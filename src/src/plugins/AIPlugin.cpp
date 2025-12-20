#include "AIPlugin.h"
#include "../core/network.h"
#include "../core/player.h"

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
    Serial.println("[AIPlugin] MVP-0: Architecture ready, all layers silent by default");
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

void AIPlugin::_processLayers(const AIContext& context) {
    uint32_t current_time = millis();
    AICandidate candidate;
    
    // Обрабатываем все слои / Process all layers
    AILayer* layers[] = { &_factsLayer, &_interpretationLayer, &_momentLayer };
    const char* layer_names[] = { "Facts", "Interpretation", "Moment" };
    
    for (size_t i = 0; i < 3; i++) {
        if (!layers[i]->isEnabled()) {
            continue;  // Слой выключен / Layer disabled
        }
        
        // Слой обрабатывает контекст / Layer processes context
        if (layers[i]->process(context, candidate)) {
            // Слой вернул кандидата / Layer returned candidate
            
            // Логируем кандидата / Log candidate
            Serial.print("[AIPlugin] ");
            Serial.print(layer_names[i]);
            Serial.print(" layer candidate: \"");
            Serial.print(candidate.text);
            Serial.print("\" (confidence=");
            Serial.print(candidate.confidence);
            Serial.print(", interval=");
            Serial.print(candidate.min_interval_ms);
            Serial.print("ms)");
            
            // Coordinator решает: показывать ли / Coordinator decides: show or not
            if (_coordinator.shouldShow(&candidate, current_time)) {
                Serial.println(" -> SHOW");
                
                // Отмечаем как показанное / Mark as shown
                _coordinator.markAsShown(&candidate, current_time);
                
                // TODO: В будущем здесь будет вывод на экран
                // TODO: In the future, screen output will be here
            } else {
                Serial.println(" -> SILENCE (filtered by coordinator)");
            }
        } else {
            // Слой молчит / Layer silent
            // Не логируем молчание для MVP-0 (слишком много шума)
            // Don't log silence for MVP-0 (too much noise)
        }
    }
}

void AIPlugin::on_track_change() {
    if (!_initialized) {
        return;
    }

    // MVP-0: Формируем контекст и обрабатываем слои
    // MVP-0: Build context and process layers
    AIContext context;
    _buildContext(context);
    
    // Логируем контекст для отладки / Log context for debugging
    Serial.print("[AIPlugin] on_track_change() - Context: ");
    Serial.print("station=\"");
    Serial.print(context.station_name);
    Serial.print("\", track=\"");
    Serial.print(context.track_title);
    Serial.print("\", artist=\"");
    Serial.print(context.artist);
    Serial.print("\", song=\"");
    Serial.print(context.song);
    Serial.print("\", playing=");
    Serial.print(context.is_playing ? "true" : "false");
    Serial.print(", hour=");
    if (context.current_hour == 255) {
        Serial.print("invalid");
    } else {
        Serial.print(context.current_hour);
    }
    Serial.println();
    
    // Обрабатываем все слои / Process all layers
    _processLayers(context);
}
