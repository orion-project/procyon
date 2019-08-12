#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include "core/OriTemplates.h"

#include <QFont>
#include <QSize>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_MOC_NAMESPACE

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
    bool useNativeMenuBar; ///< Use menu bar specfic to Ubuntu Unity or MacOS (on sceern's top).
    bool isDevMode = false; ///< Some additional features can be available in dev mode, e.g., stylesheet editor.

    QFont memoFont; ///< Default font used to desplay memo content.
    bool memoWordWrap; ///< Whether memo texts should be wrapped by default.

    void load(QSettings* s);
    void save(QSettings* s);

private:
    Settings() {}

    friend class Singleton<Settings>;
};

#endif // APP_SETTINGS_H
