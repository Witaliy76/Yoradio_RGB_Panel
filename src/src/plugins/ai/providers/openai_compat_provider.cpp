/**
 * openai_compat_provider.cpp - OpenAI-compatible provider implementation
 * Описание: Универсальный провайдер для OpenAI-compatible API, использует настройки из runtime cache
 * Description: Universal provider for OpenAI-compatible API, uses settings from runtime cache
 * Автор: W76W, 4pda.to
 * Дата: 02.01.2026
 * Версия: Yoradio RGB Panel v0.9.434m-alpha
 */

#include "openai_compat_provider.h"
#include "../../../core/config.h"  // Для aiGetRuntimeConfig / For aiGetRuntimeConfig
#include "../ai_prompt.h"  // Для загрузки промптов из SPIFFS / For loading prompts from SPIFFS (aiPromptGet, aiPromptIsAvailable)
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#ifdef ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

OpenAICompatProvider::OpenAICompatProvider() {
    // Инициализация HTTPS клиента (игнорируем проверку сертификатов для ESP32)
    // Initialize HTTPS client (ignore certificate validation for ESP32)
    _wifiSecureClient.setInsecure();
    _wifiSecureClient.setTimeout(30);  // Максимальный таймаут в секундах / Maximum timeout in seconds
}

// Нормализация пути к /chat/completions / Normalize path to /chat/completions
String OpenAICompatProvider::normalizeChatCompletionsPath(const char* basePath) {
    if (!basePath || strlen(basePath) == 0) {
        return "/v1/chat/completions";
    }
    
    String path = String(basePath);
    path.trim();
    
    // Если уже содержит "chat/completions" → вернуть как есть
    // If already contains "chat/completions" → return as is
    if (path.indexOf("chat/completions") >= 0) {
        return path;
    }
    
    // Если заканчивается на "/v1" или "/v1/" → добавить "chat/completions"
    // If ends with "/v1" or "/v1/" → add "chat/completions"
    if (path.endsWith("/v1") || path.endsWith("/v1/")) {
        if (path.endsWith("/")) {
            return path + "chat/completions";
        } else {
            return path + "/chat/completions";
        }
    }
    
    // Иначе добавить "/chat/completions" с правильным слэшем
    // Otherwise add "/chat/completions" with correct slash
    if (path.endsWith("/")) {
        return path + "chat/completions";
    } else {
        return path + "/chat/completions";
    }
}

String OpenAICompatProvider::_buildPrompt(
    const String& station_name,
    const String& artist,
    const String& song,
    const String& track_title
) {
    Serial.println("[OpenAICompatProvider] _buildPrompt() called");
    
    // СТРОГАЯ ПРОВЕРКА: промпт должен быть доступен (строгий режим, без fallback) / STRICT CHECK: prompt must be available (strict mode, no fallback)
    extern bool aiPromptIsAvailable();
    if (!aiPromptIsAvailable()) {
        Serial.println("[OpenAICompatProvider] Prompt not available, aborting request");
        return "";  // Пустой промпт → запрос не должен отправляться / Empty prompt → request should not be sent
    }
    
    // Загружаем system prompt из SPIFFS (строгий режим: только из файла) / Load system prompt from SPIFFS (strict mode: only from file)
    String system_prompt;
    bool prompt_loaded = aiPromptGet(system_prompt);  // Автоматически определяет язык из L10N_LANGUAGE / Automatically determines language from L10N_LANGUAGE
    
    // Проверка: промпт должен быть загружен из файла / Check: prompt must be loaded from file
    if (!prompt_loaded || system_prompt.isEmpty()) {
        Serial.println("[OpenAICompatProvider] Prompt not available, aborting request");
        return "";  // Пустой промпт → запрос не должен отправляться / Empty prompt → request should not be sent
    }
    
    // Формируем user prompt с данными о треке / Build user prompt with track data
    String user_prompt = "";
    if (!artist.isEmpty() && !song.isEmpty()) {
        user_prompt = "Исполнитель: " + artist + "\nТрек: " + song;
    } else if (!track_title.isEmpty()) {
        user_prompt = "Трек: " + track_title;
    } else {
        user_prompt = "Трек: неизвестен";
    }
    
    // Возвращаем system и user prompt, разделённые \n\n для последующего парсинга в _buildRequestJSON()
    // Return system and user prompt separated by \n\n for subsequent parsing in _buildRequestJSON()
    return system_prompt + "\n\n" + user_prompt;
}

String OpenAICompatProvider::_buildRequestJSON(const String& model, const String& prompt_full) {
    Serial.println("[OpenAICompatProvider] _buildRequestJSON() called");
    
    // Разделяем system и user prompt (prompt_full содержит оба через \n\n)
    // Split system and user prompt (prompt_full contains both separated by \n\n)
    int separator_pos = prompt_full.indexOf("\n\n");
    String system_prompt = (separator_pos > 0) ? prompt_full.substring(0, separator_pos) : "";
    String user_prompt = (separator_pos > 0) ? prompt_full.substring(separator_pos + 2) : prompt_full;
    
    #ifdef ESP_PLATFORM
    #define WORDS_TO_BYTES(w) ((uint32_t)(w) * sizeof(StackType_t))
    UBaseType_t stack_before = uxTaskGetStackHighWaterMark(nullptr);
    Serial.printf("[OpenAICompatProvider] Stack before JSON: %u words (~%u bytes)\n", stack_before, WORDS_TO_BYTES(stack_before));
    #undef WORDS_TO_BYTES
    #endif
    
    // Строим JSON запрос для OpenAI-compatible Chat Completions API
    // Build JSON request for OpenAI-compatible Chat Completions API
    Serial.println("[OpenAICompatProvider] Creating DynamicJsonDocument");
    DynamicJsonDocument doc(1024);
    Serial.println("[OpenAICompatProvider] DynamicJsonDocument created");
    
    doc["model"] = model;
    JsonArray messages = doc.createNestedArray("messages");
    
    // System prompt с правилами согласно манифесту
    // System prompt with rules per manifest
    JsonObject system_msg = messages.createNestedObject();
    system_msg["role"] = "system";
    system_msg["content"] = system_prompt;
    
    // User prompt с данными о треке
    // User prompt with track data
    JsonObject user_msg = messages.createNestedObject();
    user_msg["role"] = "user";
    user_msg["content"] = user_prompt;
    doc["temperature"] = 0.3;
    doc["max_tokens"] = 90;
    doc["stream"] = false;
    
    // Response format - требуем JSON (поддерживается OpenAI-compatible API)
    // Response format - require JSON (supported by OpenAI-compatible API)
    JsonObject response_format = doc.createNestedObject("response_format");
    response_format["type"] = "json_object";
    
    String json_request;
    serializeJson(doc, json_request);
    return json_request;
}

bool OpenAICompatProvider::_readHTTPResponse(String& response_body) {
    // Используем HTTPClient для чтения ответа / Use HTTPClient to read response
    response_body = _http.getString();
    
    if (response_body.length() == 0) {
        Serial.println("[OpenAICompatProvider] getString() returned empty, trying getStreamPtr()");
        WiFiClient* stream = _http.getStreamPtr();
        if (stream && stream->available()) {
            response_body = stream->readString();
            Serial.printf("[OpenAICompatProvider] Read from stream, length: %u\n", response_body.length());
        } else {
            Serial.println("[OpenAICompatProvider] Stream not available or empty");
            return false;
        }
    }
    
    if (response_body.length() == 0) {
        Serial.println("[OpenAICompatProvider] Empty response body after all attempts");
        return false;
    }
    
    return true;
}

bool OpenAICompatProvider::_makeHTTPRequest(
    const String& api_key,
    const String& model,
    const String& station_name,
    const String& artist,
    const String& song,
    const String& track_title,
    String& response_body,
    int& httpCode_out,
    String& content_type_out,
    String& content_length_out,
    String& transfer_encoding_out,
    String& content_encoding_out
) {
    Serial.println("[OpenAICompatProvider] _makeHTTPRequest() entered");
    
    // Получаем конфигурацию из runtime cache (без чтения SPIFFS)
    // Get configuration from runtime cache (without reading SPIFFS)
    AIConfig cfg;
    aiGetRuntimeConfig(cfg);
    
    // Валидация минимальных параметров / Validate minimum parameters
    if (strlen(cfg.host) == 0 || cfg.port == 0) {
        Serial.println("[OpenAICompatProvider] Invalid config: host empty or port=0");
        return false;
    }
    
    // Ограничение timeout: min 1000ms, max 30000ms
    // Timeout limit: min 1000ms, max 30000ms
    uint32_t timeout_ms = cfg.timeout_ms;
    if (timeout_ms < 1000) {
        timeout_ms = 1000;
        Serial.println("[OpenAICompatProvider] Timeout clamped to 1000ms (was too low)");
    }
    if (timeout_ms > 30000) {
        timeout_ms = 30000;
        Serial.println("[OpenAICompatProvider] Timeout clamped to 30000ms (was too high)");
    }
    
    // Закрываем предыдущее соединение если было
    // Close previous connection if exists
    _http.end();
    
    Serial.println("[OpenAICompatProvider] Calling _buildPrompt()");
    String prompt = _buildPrompt(station_name, artist, song, track_title);
    Serial.println("[OpenAICompatProvider] _buildPrompt() completed");
    
    // СТРОГАЯ ПРОВЕРКА: если промпт пустой (не загружен), abort / STRICT CHECK: if prompt empty (not loaded), abort
    if (prompt.isEmpty()) {
        Serial.println("[OpenAICompatProvider] Prompt is empty, aborting request");
        return false;
    }
    
    Serial.println("[OpenAICompatProvider] Calling _buildRequestJSON()");
    String json_request = _buildRequestJSON(model, prompt);
    Serial.println("[OpenAICompatProvider] _buildRequestJSON() completed");
    
    // Нормализация пути / Normalize path
    String normalized_path = normalizeChatCompletionsPath(cfg.path);
    
    // Формирование URL / Form URL
    // Для стандартных портов (80/443) порт не указывается в URL
    // For standard ports (80/443) port is not specified in URL
    String protocol = (cfg.port == 80) ? "http://" : "https://";
    String url;
    if (cfg.port == 80 || cfg.port == 443) {
        // Стандартные порты - не указываем порт в URL / Standard ports - don't specify port in URL
        url = protocol + String(cfg.host) + normalized_path;
    } else {
        // Нестандартные порты - указываем порт / Non-standard ports - specify port
        url = protocol + String(cfg.host) + ":" + String(cfg.port) + normalized_path;
    }
    
    Serial.printf("[OpenAICompatProvider] URL: %s\n", url.c_str());
    Serial.printf("[OpenAICompatProvider] Timeout: %u ms\n", timeout_ms);
    
    // Выбор клиента (HTTP или HTTPS) / Select client (HTTP or HTTPS)
    if (cfg.port == 80) {
        // HTTP / HTTP
        Serial.println("[OpenAICompatProvider] Using HTTP (port=80)");
        _http.begin(_wifiClient, url);
    } else {
        // HTTPS / HTTPS
        Serial.println("[OpenAICompatProvider] Using HTTPS");
        _http.begin(_wifiSecureClient, url);
    }
    
    _http.setTimeout(timeout_ms);
    _http.addHeader("Authorization", "Bearer " + api_key);
    _http.addHeader("Content-Type", "application/json");
    
    // Отправляем POST запрос / Send POST request
    int httpCode = _http.POST(json_request);
    
    Serial.printf("[OpenAICompatProvider] HTTP POST completed, code: %d\n", httpCode);
    
    // Проверяем код ответа / Check response code
    if (httpCode <= 0) {
        Serial.printf("[OpenAICompatProvider] HTTP POST failed, code: %d\n", httpCode);
        _http.end();
        return false;
    }
    
    if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_CREATED) {
        Serial.printf("[OpenAICompatProvider] HTTP error, code: %d\n", httpCode);
        response_body = _http.getString();
        _http.end();
        return false;
    }
    
    // Читаем ответ / Read response
    bool success = _readHTTPResponse(response_body);
    
    if (!success) {
        _http.end();
        return false;
    }
    
    // Сохраняем заголовки перед закрытием / Save headers before closing
    httpCode_out = httpCode;
    content_type_out = _http.hasHeader("Content-Type") ? _http.header("Content-Type") : "";
    content_length_out = _http.hasHeader("Content-Length") ? _http.header("Content-Length") : "";
    transfer_encoding_out = _http.hasHeader("Transfer-Encoding") ? _http.header("Transfer-Encoding") : "";
    content_encoding_out = _http.hasHeader("Content-Encoding") ? _http.header("Content-Encoding") : "";
    
    // Закрываем соединение / Close connection
    _http.end();
    
    return true;
}

String OpenAICompatProvider::_removeBOM(const String& str) {
    // Удаляем UTF-8 BOM (EF BB BF) если присутствует
    // Remove UTF-8 BOM (EF BB BF) if present
    if (str.length() >= 3 && 
        (unsigned char)str[0] == 0xEF && 
        (unsigned char)str[1] == 0xBB && 
        (unsigned char)str[2] == 0xBF) {
        return str.substring(3);
    }
    return str;
}

bool OpenAICompatProvider::_parseJSONResponse(const String& json_raw, LLMResponse& response, int httpCode,
                                              const String& content_type, const String& content_length,
                                              const String& transfer_encoding, const String& content_encoding) {
    // Парсинг JSON ответа от OpenAI-compatible API
    // Parse JSON response from OpenAI-compatible API
    
    Serial.println("[OpenAICompatProvider] _parseJSONResponse() called");
    
    // Удаляем BOM и пробелы / Remove BOM and whitespace
    String json = _removeBOM(json_raw);
    json.trim();
    
    Serial.printf("[OpenAICompatProvider] JSON length: %u, starts with '{': %d\n", json.length(), json.startsWith("{"));
    
    #ifdef ESP_PLATFORM
    #define WORDS_TO_BYTES(w) ((uint32_t)(w) * sizeof(StackType_t))
    UBaseType_t stack_before = uxTaskGetStackHighWaterMark(nullptr);
    Serial.printf("[OpenAICompatProvider] Stack before parse: %u words (~%u bytes)\n", stack_before, WORDS_TO_BYTES(stack_before));
    #undef WORDS_TO_BYTES
    #endif
    
    // Используем ArduinoJson для парсинга / Use ArduinoJson for parsing
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.printf("[OpenAICompatProvider] JSON deserialize error: %s\n", error.c_str());
        Serial.printf("[OpenAICompatProvider] HTTP code: %d\n", httpCode);
        
        if (!content_type.isEmpty()) {
            Serial.printf("[OpenAICompatProvider] Content-Type: %s\n", content_type.c_str());
        }
        if (!content_length.isEmpty()) {
            Serial.printf("[OpenAICompatProvider] Content-Length: %s\n", content_length.c_str());
        }
        if (!transfer_encoding.isEmpty()) {
            Serial.printf("[OpenAICompatProvider] Transfer-Encoding: %s\n", transfer_encoding.c_str());
        }
        if (!content_encoding.isEmpty()) {
            Serial.printf("[OpenAICompatProvider] Content-Encoding: %s\n", content_encoding.c_str());
        }
        
        Serial.print("[OpenAICompatProvider] Body preview (first 120 chars): ");
        Serial.println(json.length() > 120 ? json.substring(0, 120) : json);
        
        Serial.print("[OpenAICompatProvider] Body hex (first 16 bytes): ");
        for (int i = 0; i < 16 && i < json.length(); i++) {
            Serial.printf("%02X ", (unsigned char)json[i]);
        }
        Serial.println();
        
        return false;
    }
    
    Serial.println("[OpenAICompatProvider] JSON deserialized successfully");
    
    // Извлекаем content из choices[0].message.content
    if (!doc.containsKey("choices")) {
        Serial.println("[OpenAICompatProvider] No 'choices' key in JSON");
        return false;
    }
    if (!doc["choices"].is<JsonArray>()) {
        Serial.println("[OpenAICompatProvider] 'choices' is not an array");
        return false;
    }
    if (doc["choices"].size() == 0) {
        Serial.println("[OpenAICompatProvider] 'choices' array is empty");
        return false;
    }
    
    Serial.println("[OpenAICompatProvider] Found choices array");
    
    JsonObject choice = doc["choices"][0];
    if (!choice.containsKey("message")) {
        Serial.println("[OpenAICompatProvider] No 'message' key in choice");
        return false;
    }
    if (!choice["message"].containsKey("content")) {
        Serial.println("[OpenAICompatProvider] No 'content' key in message");
        return false;
    }
    
    String content = choice["message"]["content"].as<String>();
    content.trim();
    
    Serial.printf("[OpenAICompatProvider] Content length: %u\n", content.length());
    Serial.print("[OpenAICompatProvider] Content preview (first 200 chars): ");
    Serial.println(content.substring(0, 200));
    
    // Парсим внутренний JSON из content
    DynamicJsonDocument content_doc(1024);
    DeserializationError content_error = deserializeJson(content_doc, content);
    
    if (content_error) {
        Serial.printf("[OpenAICompatProvider] Content JSON deserialize error: %s\n", content_error.c_str());
        return false;
    }
    
    Serial.println("[OpenAICompatProvider] Content JSON deserialized successfully");
    
    // Проверяем обязательное поле "ok"
    if (!content_doc.containsKey("ok")) {
        Serial.println("[OpenAICompatProvider] No 'ok' key in content JSON");
        return false;
    }
    
    response.ok = content_doc["ok"].as<bool>();
    if (!response.ok) {
        return true;  // ok=false - валидный ответ, но без интерпретации
    }
    
    // Проверяем обязательные поля для успешного ответа
    if (!content_doc.containsKey("text") || !content_doc.containsKey("mode")) {
        response.ok = false;
        return true;
    }
    
    response.text = content_doc["text"].as<String>();
    response.mode = content_doc["mode"].as<String>();
    
    // confidence опционален, по умолчанию 0.5
    if (content_doc.containsKey("confidence")) {
        response.confidence = content_doc["confidence"].as<float>();
    } else {
        response.confidence = 0.5f;
    }
    
    // Валидация значений / Validate values
    if (response.text.isEmpty()) {
        response.ok = false;
        return true;
    }
    
    if (response.mode != "fact" && response.mode != "listen") {
        response.ok = false;
        return true;
    }
    
    // Удаляем переводы строк из text / Remove newlines from text
    response.text.replace("\n", " ");
    response.text.replace("\r", " ");
    response.text.trim();
    
    return true;
}

bool OpenAICompatProvider::isAvailable(const String& api_key) {
    // Ленивая проверка: фактический запрос при первой необходимости
    // Lazy check: actual request on first need
    return !api_key.isEmpty();
}

bool OpenAICompatProvider::requestInterpretation(
    const String& api_key,
    const String& model,
    const String& station_name,
    const String& artist,
    const String& song,
    const String& track_title,
    LLMResponse& response
) {
    Serial.println("[OpenAICompatProvider] requestInterpretation() called");
    
    // Проверка входных параметров / Validate input parameters
    if (api_key.isEmpty() || model.isEmpty()) {
        Serial.println("[OpenAICompatProvider] api_key or model empty");
        return false;
    }
    
    Serial.println("[OpenAICompatProvider] Parameters OK, initializing response");
    
    // Инициализируем ответ как неуспешный / Initialize response as unsuccessful
    response = LLMResponse();
    
    Serial.println("[OpenAICompatProvider] Calling _makeHTTPRequest()");
    
    // Выполняем HTTP/HTTPS запрос / Perform HTTP/HTTPS request
    String response_body;
    int httpCode;
    String content_type, content_length, transfer_encoding, content_encoding;
    
    if (!_makeHTTPRequest(api_key, model, station_name, artist, song, track_title, 
                         response_body, httpCode, content_type, content_length, 
                         transfer_encoding, content_encoding)) {
        Serial.println("[OpenAICompatProvider] _makeHTTPRequest() failed");
        return false;
    }
    
    Serial.printf("[OpenAICompatProvider] Response body length: %u\n", response_body.length());
    
    // Парсим JSON ответ / Parse JSON response
    if (!_parseJSONResponse(response_body, response, httpCode, content_type, content_length, 
                            transfer_encoding, content_encoding)) {
        Serial.println("[OpenAICompatProvider] _parseJSONResponse() failed");
        return false;
    }
    
    Serial.println("[OpenAICompatProvider] _parseJSONResponse() succeeded");
    Serial.println("[OpenAICompatProvider] requestInterpretation() succeeded");
    return true;
}

