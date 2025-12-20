#ifndef AIPLUGIN_H
#define AIPLUGIN_H

#include "../pluginsManager/pluginsManager.h"
#include "../core/config.h"

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
    virtual ~AIPlugin() {}

    // Публичный метод для явной регистрации плагина
    // Public method for explicit plugin registration
    void init();

    // Переопределяем обработчики событий / Override event handlers
    virtual void on_setup() override;
    virtual void on_track_change() override;

private:
    bool _initialized;
};

#endif // AIPLUGIN_H
