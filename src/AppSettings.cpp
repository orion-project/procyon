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

#define LOAD(option, type)\
    option = s.settings()->value(QStringLiteral(#option)).to ## type()

#define LOAD_DEF(option, type, default_value)\
    option = s.settings()->value(QStringLiteral(#option), default_value).to ## type()

#define SAVE(option)\
    s.settings()->setValue(QStringLiteral(#option), option)

void Settings::load()
{
    Ori::Settings s;

    s.beginGroup("View");
    LOAD_DEF(useNativeMenuBar, Bool, true);
    LOAD_DEF(baseColor, String, "#dadbde");
}

void Settings::save()
{
    Ori::Settings s;

    s.beginGroup("View");
    SAVE(useNativeMenuBar);
    SAVE(baseColor);
}
