#include "AppSettings.h"

#include "Utils.h"

#include "tools/OriSettings.h"

//------------------------------------------------------------------------------
//                              AppSettings::Option(s)
//------------------------------------------------------------------------------

AppSettings::Option::~Option()
{
}

AppSettings::Options::Options(Options& other)
{
    _options = std::move(other._options);
}

AppSettings::Options::Options(Options&& other)
{
    _options = std::move(other._options);
}

AppSettings::Options::Options(const std::initializer_list<Option*> options)
{
    _options = options;
}

AppSettings::Options::~Options()
{
    for (auto option : _options) delete option;
}

template <typename T> class OptionSpec : public AppSettings::Option
{
public:
    QVariant value() const override { return QVariant::fromValue(*(reinterpret_cast<T*>(_value))); }
    void setValue(const QVariant& v) override { *reinterpret_cast<T*>(_value) = v.value<T>(); }

private:
    OptionSpec(const QString& category, const QString& name, const QString& title,
               const QString& description, const QVariant& defaultValue, T* value) : Option()
    {
        this->category = category;
        this->name = name;
        this->title = title;
        this->description = description;
        this->defaultValue = defaultValue;
        this->_value = value;
    }
    friend class AppSettings;
};

//------------------------------------------------------------------------------
//                              SettingsListener
//------------------------------------------------------------------------------

AppSettingsListener::AppSettingsListener()
{
    AppSettings::instance().registerListener(this);
}

AppSettingsListener::~AppSettingsListener()
{
    AppSettings::instance().unregisterListener(this);
}

//------------------------------------------------------------------------------
//                               AppSettings
//------------------------------------------------------------------------------

void AppSettings::load(QSettings* s)
{
    auto opts = options();
    for (auto option : opts.items())
    {
        Ori::SettingsGroup group(s, option->category);
        option->setValue(s->value(option->name, option->defaultValue));
    }
}

void AppSettings::save(QSettings* s)
{
    auto opts = options();
    for (auto option : opts.items())
    {
        Ori::SettingsGroup group(s, option->category);
        s->setValue(option->name, option->value());
    }
}

QString AppSettings::markdownCss()
{
    if (_markdownCss.isEmpty())
        _markdownCss = loadTextFromResource(":/style/markdown_css");
    return _markdownCss;
}

void AppSettings::updateMarkdownCss(const QString css)
{
    _markdownCss = css;
    NOTIFY_LISTENERS_1(optionChanged, AppSettingsOption::MARKDOWN_CSS);
}

AppSettings::Options AppSettings::options()
{
    return {
         new OptionSpec<QFont>(
                    "Memo",
                    "defaultFont",
                    "Default memo font",
                    "Default font used for displaying memo content",
                    QFont("Arial", 12),
                    &memoFont
                    ),
        new OptionSpec<bool>(
                    "Memo",
                    "defaultWordWrap",
                    "Word-wrap memo by default",
                    "Whether memo texts should be wrapped by default",
                    false,
                    &memoWordWrap
                    ),
        new OptionSpec<bool>(
                    "View",
                    "useNativeMenuBar",
                    "Use native menu bar",
                    "Use menu bar specfic to Ubuntu Unity or MacOS (on sceern's top)",
            #ifdef Q_OS_WIN
                    false,
            #else
                    true,
            #endif
                    &useNativeMenuBar
                    )
    };
}
