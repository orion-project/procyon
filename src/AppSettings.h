#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include "core/OriTemplates.h"

#include <QFont>
#include <QSize>
#include <QVariant>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

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

class AppSettings : public Ori::Singleton<AppSettings>,
                    public Ori::Notifier<AppSettingsListener>
{
public:
    class Option
    {
    public:
        QString name;
        QString title;
        QString category;
        QString description;
        QVariant defaultValue;
        virtual ~Option();
        virtual QVariant value() const = 0;
        virtual void setValue(const QVariant& v) = 0;
    protected:
        void* _value;
        Option() {}
    };

    class Options
    {
    public:
        Options(Options& other);
        Options(Options&& other);
        Options(const std::initializer_list<Option*> options);
        ~Options();
        const QVector<Option*>& items() const { return _options; }
        Options operator =(Options& other) { return Options(other); }
        Options operator =(Options&& other) { return Options(other); }
    private:
        QVector<Option*> _options;
    };

public:
    bool useNativeMenuBar; ///< Use menu bar specfic to Ubuntu Unity or MacOS (on sceern's top).
    bool isDevMode = false; ///< Some additional features can be available in dev mode, e.g., stylesheet editor.

    QFont memoFont; ///< Default font used to desplay memo content.
    bool memoWordWrap; ///< Whether memo texts should be wrapped by default.

    QString markdownCss();
    void updateMarkdownCss(const QString css);

    void load(QSettings* s);
    void save(QSettings* s);

    Options options();

private:
    AppSettings() {}
    ~AppSettings() = delete;

    QString _markdownCss;

    friend class Singleton<AppSettings>;
};

#endif // APP_SETTINGS_H
