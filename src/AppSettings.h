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
    bool useNativeMenuBar;    ///< Use menu bar specfic to Ubuntu Unity or MacOS (on sceern's top).
    bool isDevMode = false;

    /// General window color, it can be set slightly different than the default, for example,
    /// to make it better match the window borders color depending on the desktop theme.
    QString baseColor;

    void load();
    void save();

private:
    Settings() {}

    friend class Singleton<Settings>;
};

#endif // APP_SETTINGS_H
