#include "AppSettings.h"
#include "tools/OriSettings.h"

//------------------------------------------------------------------------------
//                              SettingsListener
//------------------------------------------------------------------------------

SettingsListener::SettingsListener()
{
    Settings::instance().registerListener(this);
}

SettingsListener::~SettingsListener()
{
    Settings::instance().unregisterListener(this);
}


//------------------------------------------------------------------------------
//                               Settings
//------------------------------------------------------------------------------

#define LOAD(option, type, default_value)\
    option = s->value(QStringLiteral(#option), default_value).to ## type()

#define SAVE(option)\
    s->setValue(QStringLiteral(#option), option)

void Settings::load(QSettings* s)
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

void Settings::save(QSettings* s)
{
    Ori::SettingsGroup group(s, "View");
    SAVE(useNativeMenuBar);
    SAVE(memoWordWrap);
    SAVE(memoFont);
}
