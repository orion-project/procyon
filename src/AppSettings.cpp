#include "AppSettings.h"

#include "Utils.h"

#include "tools/OriSettings.h"

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
//                               Settings
//------------------------------------------------------------------------------

#define LOAD(option, type, default_value)\
    option = s->value(QStringLiteral(#option), default_value).to ## type()

#define SAVE(option)\
    s->setValue(QStringLiteral(#option), option)

void AppSettings::load(QSettings* s)
{
    bool defaultUseNativeMenuBar =
#ifdef Q_OS_WIN
        false;
#else
        true;
#endif

    Ori::SettingsGroup group(s, "View");
    LOAD(useNativeMenuBar, Bool, defaultUseNativeMenuBar);
    LOAD(memoWordWrap, Bool, false);
    memoFont = qvariant_cast<QFont>(s->value("memoFont", QFont("Arial", 12)));
}

void AppSettings::save(QSettings* s)
{
    Ori::SettingsGroup group(s, "View");
    SAVE(useNativeMenuBar);
    SAVE(memoWordWrap);
    SAVE(memoFont);
}

QString AppSettings::markdownCss()
{
    if (_markdownCss.isEmpty())
        _markdownCss = loadTextFromResource(":/docs/markdown_css");
    return _markdownCss;
}

void AppSettings::updateMarkdownCss(const QString css)
{
    _markdownCss = css;
    NOTIFY_LISTENERS_1(optionChanged, AppSettingsOption::MARKDOWN_CSS);
}
