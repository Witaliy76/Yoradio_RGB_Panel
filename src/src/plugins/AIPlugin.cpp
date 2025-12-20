#include "AIPlugin.h"

extern Config config;

AIPlugin::AIPlugin() : _initialized(false) {
    // Не регистрируем плагин в конструкторе - регистрация будет явной в setup()
    // Don't register plugin in constructor - registration will be explicit in setup()
}

void AIPlugin::init() {
    // Явная регистрация плагина
    // Explicit plugin registration
    registerPlugin();
}

void AIPlugin::on_setup() {
    Serial.println("[AIPlugin] on_setup() called - AI plugin initialized");
    _initialized = true;
}

void AIPlugin::on_track_change() {
    if (!_initialized) {
        return;
    }

    // Вывод информации о треке для отладки / Track information output for debugging
    // В будущем здесь будет AI-логика согласно манифестам
    // In the future, AI logic will be here according to manifests
    Serial.print("[AIPlugin] on_track_change() called - ");
    
    if (strlen(config.station.title) > 0) {
        Serial.print("Title: ");
        Serial.print(config.station.title);
    } else {
        Serial.print("(no title)");
    }
    
    Serial.print(" | Station: ");
    if (strlen(config.station.name) > 0) {
        Serial.print(config.station.name);
    } else {
        Serial.print("(no station name)");
    }
    
    Serial.println();
}
