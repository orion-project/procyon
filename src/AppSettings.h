#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include "core/OriTemplates.h"

#include <QFont>
#include <QSize>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_MOC_NAMESPACE

enum class AppSettingsOption
{
    MARKDOWN_CSS
};

class AppSettingsListener
{
public:
    AppSettingsListener();
    virtual ~AppSettingsListener();

    virtual void settingsChanged() {}
    virtual void optionChanged(AppSettingsOption) {}
};

class AppSettings :
        public Singleton<AppSettings>,
        public Notifier<AppSettingsListener>
{
public:
    bool useNativeMenuBar; ///< Use menu bar specfic to Ubuntu Unity or MacOS (on sceern's top).
    bool isDevMode = false; ///< Some additional features can be available in dev mode, e.g., stylesheet editor.

    QFont memoFont; ///< Default font used to desplay memo content.
    bool memoWordWrap; ///< Whether memo texts should be wrapped by default.

    QString markdownCss();
    void updateMarkdownCss(const QString css);

    void load(QSettings* s);
    void save(QSettings* s);

private:
    AppSettings() {}

    QString _markdownCss;

    friend class Singleton<AppSettings>;
};

#endif // APP_SETTINGS_H
