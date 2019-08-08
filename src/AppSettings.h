#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include "core/OriTemplates.h"
#include <QSize>

class SettingsListener
{
public:
    SettingsListener();
    virtual ~SettingsListener();

    virtual void settingsChanged() {}
};

class Settings :
        public Singleton<Settings>,
        public Notifier<SettingsListener>
{
public:
#ifndef Q_OS_WIN
    bool useNativeMenuBar;    ///< Use menu bar specfic to Ubuntu Unity or MacOS (on sceern's top).
#endif
    bool isDevMode = false;

    void load();
    void save();

private:
    Settings() {}

    friend class Singleton<Settings>;
};

#endif // APP_SETTINGS_H
