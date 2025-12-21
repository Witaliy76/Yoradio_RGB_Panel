#include "deepseek_provider.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <HTTPClient.h>
#ifdef ESP_PLATFORM
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

const char* DeepSeekProvider::API_HOST = "api.deepseek.com";
const int DeepSeekProvider::API_PORT = 443;  // HTTPS
const char* DeepSeekProvider::API_PATH = "/v1/chat/completions";
const unsigned long DeepSeekProvider::REQUEST_TIMEOUT_MS = 8000;  // 8 секунд (уменьшено для меньшего влияния на аудио)

DeepSeekProvider::DeepSeekProvider() {
    // Инициализация HTTPS клиента (игнорируем проверку сертификатов для ESP32)
    // Initialize HTTPS client (ignore certificate validation for ESP32)
    _wifiClient.setInsecure();
    _wifiClient.setTimeout(REQUEST_TIMEOUT_MS / 1000);  // timeout в секундах / timeout in seconds
}

String DeepSeekProvider::_buildPrompt(
    const String& station_name,
    const String& artist,
    const String& song,
    const String& track_title
) {
    Serial.println("[DeepSeekProvider] _buildPrompt() called");
    // Формируем промпт строго по манифесту
    // Build prompt strictly per manifest
    
    String system_prompt =
        "Ты — тихий музыкальный информер. Отвечай ТОЛЬКО JSON, без пояснений.\n"
        "Формат: {\"ok\": true, \"mode\": \"fact\"|\"listen\", \"text\": \"...\", \"confidence\": 0.0-1.0}\n"
        "или {\"ok\": false} если лучше промолчать.\n\n"
        "ПРАВИЛА (строго):\n"
        "1) mode=\"fact\" — только если факт общеизвестный и легко проверяемый, уверен на 0.85+.\n"
        "   ВЫБОР ФАКТА (приоритет):\n"
        "   a) Сначала попробуй дать интересный общеизвестный факт, НЕ сводящийся к году релиза.\n"
        "      Примеры: прорывной хит, известный саундтрек, знаковая история записи.\n"
        "   b) Если такого факта нет, тогда допускается год/дата релиза как запасной вариант.\n"
        "   c) Если нет даже этого — используй mode=\"listen\" или ok=false.\n"
        "   Важно: НЕ выбирай год релиза как первый вариант ответа.\n\n"
        "   ДОПУСТИМЫЕ fact (только если уверен на 0.85+):\n"
        "   - необычный факт о создании/записи трека или альбома (production trivia: место записи, история записи)\n"
        "   - известный саундтрек/использование в культовом фильме/сериале\n"
        "   Запрещено указывать саундтрек фильма,\n"
        "   если трек не входит в официальный лицензированный саундтрек\n"
        "   и это не является широко известным фактом.\n"
        "   При малейшем сомнении — НЕ использовать film soundtrack как fact.\n"
        "   Если факт основан на культурной ассоциации, а не на документально подтверждённом источнике,\n"
        "   НЕ использовать его как fact.\n\n"
        "   ОПАСНЫЕ fact (награды/чарты/Зал славы/продажи) — ТОЛЬКО если уверен на 0.95+:\n"
        "   - крупные награды или номинации (например, Grammy/Brit Awards и т.п.)\n"
        "   - достижения в чартах (#1, прорывной хит)\n"
        "   - включение в Зал славы, крупные продажи, рекорды\n"
        "   Если уверенность ниже 0.95 или есть риск перепутать детали — НЕ пиши это как fact.\n"
        "   Вместо этого выбери mode=\"listen\" или ok=false.\n"
        "   Если пишешь про награду/чарт/рекорд — формулируй максимально конкретно и коротко,\n"
        "   без смешивания разных фактов в одну фразу.\n\n"
        "   ЕСЛИ ИСПОЛНИТЕЛЬ ШИРОКО ИЗВЕСТЕН:\n"
        "   Разрешено выбрать ЛЮБОЙ один общеизвестный и легко проверяемый факт,\n"
        "   даже если существует несколько вариантов.\n"
        "   Не нужно искать \"самый лучший\" факт — достаточно безопасного и короткого.\n"
        "   Примеры: включение в Зал славы, крупные продажи, культовый статус,\n"
        "   известное появление в фильме.\n"
        "   Формулировки \"известен(на) тем что\" допустимы\n"
        "   только для описания музыкального приёма или звучания,\n"
        "   а не жизненного пути или сотрудничеств.\n"
        "   Запрещено для fact: биографии, сотрудничества,\n"
        "   коллаборации и любые спорные или нишевые утверждения.\n"
        "ВАЖНОЕ РАЗДЕЛЕНИЕ:\n"
        "- Любое описание звучания/настроения/темпа/вокала/жанра/подачи/атмосферы — это mode=\"listen\".\n"
        "- mode=\"fact\" НЕ должен содержать описания звучания. Fact — только проверяемые сведения вне музыки (релиз/хит/саундтрек/награда).\n"
        "Если сомневаешься между fact и listen — выбирай listen.\n"
        "2) Если нет безопасного факта — используй mode=\"listen\" или ok=false.\n"
        "3) mode=\"listen\" — одна короткая нейтральная фраза о настроении, темпе или звучании музыки.\n"
        "   Без советов, без обращений, без оценок.\n"
        "4) Запрещено упоминать радиостанцию, писать \"сейчас играет\",\n"
        "   пересказывать название трека или исполнителя.\n"
        "5) Одна строка, по-русски, без эмодзи. Используй только кавычки \"\" и дефис '-'.\n";
    
    String user_prompt = "";
    if (!artist.isEmpty() && !song.isEmpty()) {
        user_prompt = "Исполнитель: " + artist + "\nТрек: " + song;
    } else if (!track_title.isEmpty()) {
        user_prompt = "Трек: " + track_title;
    } else {
        user_prompt = "Трек: неизвестен";
    }
    
    // System prompt передаётся отдельно в JSON запросе
    // User prompt - это данные о треке
    return system_prompt + user_prompt;
}

String DeepSeekProvider::_buildRequestJSON(const String& model, const String& prompt_full) {
    Serial.println("[DeepSeekProvider] _buildRequestJSON() called");
    
    // Разделяем system и user prompt (prompt_full содержит оба через \n\n)
    // Split system and user prompt (prompt_full contains both separated by \n\n)
    int separator_pos = prompt_full.indexOf("\n\n");
    String system_prompt = (separator_pos > 0) ? prompt_full.substring(0, separator_pos) : "";
    String user_prompt = (separator_pos > 0) ? prompt_full.substring(separator_pos + 2) : prompt_full;
    
    #ifdef ESP_PLATFORM
    #define WORDS_TO_BYTES(w) ((uint32_t)(w) * sizeof(StackType_t))
    UBaseType_t stack_before = uxTaskGetStackHighWaterMark(nullptr);
    Serial.printf("[DeepSeekProvider] Stack before JSON: %u words (~%u bytes)\n", stack_before, WORDS_TO_BYTES(stack_before));
    #undef WORDS_TO_BYTES
    #endif
    
    // Строим JSON запрос для DeepSeek Chat Completions API
    // Build JSON request for DeepSeek Chat Completions API
    // Используем DynamicJsonDocument вместо StaticJsonDocument для уменьшения использования стека
    // Use DynamicJsonDocument instead of StaticJsonDocument to reduce stack usage
    Serial.println("[DeepSeekProvider] Creating DynamicJsonDocument");
    DynamicJsonDocument doc(1024);
    Serial.println("[DeepSeekProvider] DynamicJsonDocument created");
    
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
    
    // Response format - требуем JSON
    JsonObject response_format = doc.createNestedObject("response_format");
    response_format["type"] = "json_object";
    
    String json_request;
    serializeJson(doc, json_request);
    return json_request;
}

bool DeepSeekProvider::_readHTTPResponse(String& response_body) {
    // Используем HTTPClient для чтения ответа / Use HTTPClient to read response
    // HTTPClient автоматически обрабатывает заголовки / HTTPClient automatically handles headers
    response_body = _http.getString();
    
    if (response_body.length() == 0) {
        Serial.println("[DeepSeekProvider] getString() returned empty, trying getStreamPtr()");
        // Если getString() вернул пусто, пробуем читать через stream
        // If getString() returned empty, try reading through stream
        WiFiClient* stream = _http.getStreamPtr();
        if (stream && stream->available()) {
            response_body = stream->readString();
            Serial.printf("[DeepSeekProvider] Read from stream, length: %u\n", response_body.length());
        } else {
            Serial.println("[DeepSeekProvider] Stream not available or empty");
            return false;
        }
    }
    
    if (response_body.length() == 0) {
        Serial.println("[DeepSeekProvider] Empty response body after all attempts");
        return false;
    }
    
    return true;
}

bool DeepSeekProvider::_makeHTTPRequest(
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
    Serial.println("[DeepSeekProvider] _makeHTTPRequest() entered");
    
    // Закрываем предыдущее соединение если было
    // Close previous connection if exists
    _http.end();
    
    Serial.println("[DeepSeekProvider] Calling _buildPrompt()");
    // Формируем промпт
    String prompt = _buildPrompt(station_name, artist, song, track_title);
    Serial.println("[DeepSeekProvider] _buildPrompt() completed");
    
    Serial.println("[DeepSeekProvider] Calling _buildRequestJSON()");
    // Строим JSON запрос
    String json_request = _buildRequestJSON(model, prompt);
    Serial.println("[DeepSeekProvider] _buildRequestJSON() completed");
    
    // Подключаемся к серверу
    String url = "https://" + String(API_HOST) + String(API_PATH);
    _http.begin(_wifiClient, url);
    _http.setTimeout(REQUEST_TIMEOUT_MS);
    _http.addHeader("Authorization", "Bearer " + api_key);
    _http.addHeader("Content-Type", "application/json");
    
    // Отправляем POST запрос
    int httpCode = _http.POST(json_request);
    
    // Логируем HTTP код всегда
    // Log HTTP code always
    Serial.printf("[DeepSeekProvider] HTTP POST completed, code: %d\n", httpCode);
    
    // Проверяем код ответа
    if (httpCode <= 0) {
        Serial.printf("[DeepSeekProvider] HTTP POST failed, code: %d\n", httpCode);
        _http.end();
        return false;
    }
    
    if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_CREATED) {
        Serial.printf("[DeepSeekProvider] HTTP error, code: %d\n", httpCode);
        response_body = _http.getString();
        _http.end();
        return false;
    }
    
    // Читаем ответ
    bool success = _readHTTPResponse(response_body);
    
    if (!success) {
        _http.end();
        return false;
    }
    
    // Сохраняем заголовки перед закрытием для логирования ошибок парсинга
    // Save headers before closing for parsing error logging
    httpCode_out = httpCode;
    content_type_out = _http.hasHeader("Content-Type") ? _http.header("Content-Type") : "";
    content_length_out = _http.hasHeader("Content-Length") ? _http.header("Content-Length") : "";
    transfer_encoding_out = _http.hasHeader("Transfer-Encoding") ? _http.header("Transfer-Encoding") : "";
    content_encoding_out = _http.hasHeader("Content-Encoding") ? _http.header("Content-Encoding") : "";
    
    // Закрываем соединение
    _http.end();
    
    return true;
}

String DeepSeekProvider::_removeBOM(const String& str) {
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

bool DeepSeekProvider::_parseJSONResponse(const String& json_raw, LLMResponse& response, int httpCode,
                                          const String& content_type, const String& content_length,
                                          const String& transfer_encoding, const String& content_encoding) {
    // Парсинг JSON ответа от DeepSeek API
    // Parse JSON response from DeepSeek API
    // DeepSeek возвращает формат: {"choices":[{"message":{"content":"{...}"}}]}
    // Нам нужен внутренний JSON из content: {"ok": true, "interpretation": "...", "mode": "fact|listen", "confidence": 0.0}
    
    Serial.println("[DeepSeekProvider] _parseJSONResponse() called");
    
    // Удаляем BOM и пробелы / Remove BOM and whitespace
    String json = _removeBOM(json_raw);
    json.trim();
    
    Serial.printf("[DeepSeekProvider] JSON length: %u, starts with '{': %d\n", json.length(), json.startsWith("{"));
    
    // Логируем для диагностики (но не блокируем если не начинается с '{')
    // Log for diagnostics (but don't block if doesn't start with '{')
    
    #ifdef ESP_PLATFORM
    #define WORDS_TO_BYTES(w) ((uint32_t)(w) * sizeof(StackType_t))
    UBaseType_t stack_before = uxTaskGetStackHighWaterMark(nullptr);
    Serial.printf("[DeepSeekProvider] Stack before parse: %u words (~%u bytes)\n", stack_before, WORDS_TO_BYTES(stack_before));
    #undef WORDS_TO_BYTES
    #endif
    
    // Используем ArduinoJson для парсинга
    // Use ArduinoJson for parsing
    // Используем DynamicJsonDocument вместо StaticJsonDocument для уменьшения использования стека
    // Use DynamicJsonDocument instead of StaticJsonDocument to reduce stack usage
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        // При ошибке парсинга логируем подробную информацию
        // On parse error log detailed information
        Serial.printf("[DeepSeekProvider] JSON deserialize error: %s\n", error.c_str());
        Serial.printf("[DeepSeekProvider] HTTP code: %d\n", httpCode);
        
        // Логируем заголовки / Log headers
        if (!content_type.isEmpty()) {
            Serial.printf("[DeepSeekProvider] Content-Type: %s\n", content_type.c_str());
        }
        if (!content_length.isEmpty()) {
            Serial.printf("[DeepSeekProvider] Content-Length: %s\n", content_length.c_str());
        }
        if (!transfer_encoding.isEmpty()) {
            Serial.printf("[DeepSeekProvider] Transfer-Encoding: %s\n", transfer_encoding.c_str());
        }
        if (!content_encoding.isEmpty()) {
            Serial.printf("[DeepSeekProvider] Content-Encoding: %s\n", content_encoding.c_str());
        }
        
        // Первые 120 символов body / First 120 characters of body
        Serial.print("[DeepSeekProvider] Body preview (first 120 chars): ");
        Serial.println(json.length() > 120 ? json.substring(0, 120) : json);
        
        // Первые 16 байт body в hex / First 16 bytes of body in hex
        Serial.print("[DeepSeekProvider] Body hex (first 16 bytes): ");
        for (int i = 0; i < 16 && i < json.length(); i++) {
            Serial.printf("%02X ", (unsigned char)json[i]);
        }
        Serial.println();
        
        return false;  // Ошибка парсинга / Parse error
    }
    
    Serial.println("[DeepSeekProvider] JSON deserialized successfully");
    
    // Извлекаем content из choices[0].message.content
    if (!doc.containsKey("choices")) {
        Serial.println("[DeepSeekProvider] No 'choices' key in JSON");
        return false;
    }
    if (!doc["choices"].is<JsonArray>()) {
        Serial.println("[DeepSeekProvider] 'choices' is not an array");
        return false;
    }
    if (doc["choices"].size() == 0) {
        Serial.println("[DeepSeekProvider] 'choices' array is empty");
        return false;
    }
    
    Serial.println("[DeepSeekProvider] Found choices array");
    
    JsonObject choice = doc["choices"][0];
    if (!choice.containsKey("message")) {
        Serial.println("[DeepSeekProvider] No 'message' key in choice");
        return false;
    }
    if (!choice["message"].containsKey("content")) {
        Serial.println("[DeepSeekProvider] No 'content' key in message");
        return false;
    }
    
    String content = choice["message"]["content"].as<String>();
    content.trim();
    
    Serial.printf("[DeepSeekProvider] Content length: %u\n", content.length());
    Serial.print("[DeepSeekProvider] Content preview (first 200 chars): ");
    Serial.println(content.substring(0, 200));
    
    // Парсим внутренний JSON из content
    // Используем DynamicJsonDocument вместо StaticJsonDocument для уменьшения использования стека
    // Use DynamicJsonDocument instead of StaticJsonDocument to reduce stack usage
    DynamicJsonDocument content_doc(1024);
    DeserializationError content_error = deserializeJson(content_doc, content);
    
    if (content_error) {
        Serial.printf("[DeepSeekProvider] Content JSON deserialize error: %s\n", content_error.c_str());
        return false;  // Ошибка парсинга внутреннего JSON
    }
    
    Serial.println("[DeepSeekProvider] Content JSON deserialized successfully");
    
    // Проверяем обязательное поле "ok"
    if (!content_doc.containsKey("ok")) {
        Serial.println("[DeepSeekProvider] No 'ok' key in content JSON");
        return false;
    }
    
    response.ok = content_doc["ok"].as<bool>();
    if (!response.ok) {
        return true;  // ok=false - валидный ответ, но без интерпретации
    }
    
    // Проверяем обязательные поля для успешного ответа (text вместо interpretation согласно манифесту)
    // Check required fields for successful response (text instead of interpretation per manifest)
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
    
    // Валидация значений
    // Validate values
    if (response.text.isEmpty()) {
        response.ok = false;
        return true;
    }
    
    if (response.mode != "fact" && response.mode != "listen") {
        response.ok = false;  // Невалидный mode - молчим / Invalid mode - silence
        return true;
    }
    
    // Удаляем переводы строк из text
    // Remove newlines from text
    response.text.replace("\n", " ");
    response.text.replace("\r", " ");
    response.text.trim();
    
    return true;
}

bool DeepSeekProvider::isAvailable(const String& api_key) {
    // Ленивая проверка: фактический запрос при первой необходимости
    // Lazy check: actual request on first need
    // Пока возвращаем true если ключ не пустой
    // For now return true if key is not empty
    return !api_key.isEmpty();
}

bool DeepSeekProvider::requestInterpretation(
    const String& api_key,
    const String& model,
    const String& station_name,
    const String& artist,
    const String& song,
    const String& track_title,
    LLMResponse& response
) {
    Serial.println("[DeepSeekProvider] requestInterpretation() called");
    
    // Проверка входных параметров
    // Validate input parameters
    if (api_key.isEmpty() || model.isEmpty()) {
        Serial.println("[DeepSeekProvider] api_key or model empty");
        return false;
    }
    
    Serial.println("[DeepSeekProvider] Parameters OK, initializing response");
    
    // Инициализируем ответ как неуспешный
    // Initialize response as unsuccessful
    response = LLMResponse();
    
    Serial.println("[DeepSeekProvider] Calling _buildPrompt()");
    
    // Выполняем HTTPS запрос
    // Perform HTTPS request
    Serial.println("[DeepSeekProvider] Calling _makeHTTPRequest()");
    String response_body;
    int httpCode;
    String content_type, content_length, transfer_encoding, content_encoding;
    
    if (!_makeHTTPRequest(api_key, model, station_name, artist, song, track_title, 
                         response_body, httpCode, content_type, content_length, 
                         transfer_encoding, content_encoding)) {
        Serial.println("[DeepSeekProvider] _makeHTTPRequest() failed");
        return false;  // Ошибка запроса / Request error
    }
    
    Serial.printf("[DeepSeekProvider] Response body length: %u\n", response_body.length());
    
    // Парсим JSON ответ
    // Parse JSON response
    if (!_parseJSONResponse(response_body, response, httpCode, content_type, content_length, 
                            transfer_encoding, content_encoding)) {
        Serial.println("[DeepSeekProvider] _parseJSONResponse() failed");
        // Детальная информация об ошибке уже залогирована в _parseJSONResponse()
        // Detailed error information already logged in _parseJSONResponse()
        return false;  // Ошибка парсинга / Parse error
    }
    
    Serial.println("[DeepSeekProvider] _parseJSONResponse() succeeded");
    
    Serial.println("[DeepSeekProvider] requestInterpretation() succeeded");
    return true;  // Успешно (но response.ok может быть false если нет данных)
}

