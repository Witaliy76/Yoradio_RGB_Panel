#ifndef DEEPSEEK_PROVIDER_H
#define DEEPSEEK_PROVIDER_H

/**
 * deepseek_provider.h - Заголовочный файл провайдера DeepSeek
 * Описание: Реализация LLMProvider для DeepSeek API (Chat Completions)
 * Автор: W76W, 4pda.to
 * Дата: 21.12.2025
 * Версия: Yoradio RGB Panel v0.9.434m-alpha
 */

#include "llm_provider.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

/**
 * DeepSeekProvider - реализация LLMProvider для DeepSeek API
 * DeepSeekProvider - LLMProvider implementation for DeepSeek API
 * 
 * Использует Chat Completions API
 * Uses Chat Completions API
 */
class DeepSeekProvider : public LLMProvider {
private:
    static const char* API_HOST;
    static const int API_PORT;
    static const char* API_PATH;
    static const unsigned long REQUEST_TIMEOUT_MS;
    
    HTTPClient _http;  // HTTP клиент / HTTP client
    WiFiClientSecure _wifiClient;  // HTTPS клиент / HTTPS client
    
    // Вспомогательные методы / Helper methods
    bool _makeHTTPRequest(
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
    );
    
    String _buildRequestJSON(const String& model, const String& prompt);
    bool _readHTTPResponse(String& response_body);
    
    // Удаление UTF-8 BOM из строки / Remove UTF-8 BOM from string
    String _removeBOM(const String& str);
    
    bool _parseJSONResponse(const String& json, LLMResponse& response, int httpCode, 
                           const String& content_type, const String& content_length,
                           const String& transfer_encoding, const String& content_encoding);
    String _buildPrompt(const String& station_name, const String& artist, const String& song, const String& track_title);
    
public:
    DeepSeekProvider();
    virtual ~DeepSeekProvider() {}
    
    virtual bool isAvailable(const String& api_key) override;
    virtual bool requestInterpretation(
        const String& api_key,
        const String& model,
        const String& station_name,
        const String& artist,
        const String& song,
        const String& track_title,
        LLMResponse& response
    ) override;
    
    virtual const char* getName() const override { return "DeepSeek"; }
};

#endif // DEEPSEEK_PROVIDER_H

