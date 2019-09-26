#include "MainWindow.h"

#include "AppSettings.h"
#include "Utils.h"

#include "tools/OriDebug.h"
#include "tools/OriSettings.h"

#include <QApplication>
#include <QDebug>
#include <QStyleFactory>
#include <QCommandLineParser>
#include <QMessageBox>
#include <QRegularExpression>

QString loadStyleSheet(QSettings* s)
{
    QString styleSheet = loadTextFromResource(":/style/app_main");

    Ori::SettingsGroup group(s, "Theme");

    // General window color, it can be set slightly different than the default, for example,
    // to make it better match the window borders color depending on the desktop theme.
    styleSheet.replace("$base-color", s->value("baseColor", "#dadbde").toString());

    auto options = QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption;
    QRegularExpression propWin(QStringLiteral("^\\s*windows:(.*)$"), options);
    QRegularExpression propLinux(QStringLiteral("^\\s*linux:(.*)$"), options);
    QRegularExpression propMacos(QStringLiteral("^\\s*macos:(.*)$"), options);
#if defined(Q_OS_WIN)
    styleSheet.replace(propWin, QStringLiteral("\\1"));
    styleSheet.remove(propLinux);
    styleSheet.remove(propMacos);
#elif defined(Q_OS_UNIX)
    styleSheet.replace(propLinux, QStringLiteral("\\1"));
    styleSheet.remove(propWin);
    styleSheet.remove(propMacos);
#elif defined(Q_OS_MAC)
    styleSheet.replace(propMacos, QStringLiteral("\\1"));
    styleSheet.remove(propWin);
    styleSheet.remove(propLinux);
#endif

    return styleSheet;
}

bool processCommandLine()
{
    QCommandLineParser parser;
    auto optionHelp = parser.addHelpOption();
    auto optionVersion = parser.addVersionOption();
    QCommandLineOption optionDevMode("dev"); optionDevMode.setFlags(QCommandLineOption::HiddenFromHelp);
    QCommandLineOption optionConsole("console"); optionConsole.setFlags(QCommandLineOption::HiddenFromHelp);
    parser.addOptions({optionDevMode, optionConsole});

    if (!parser.parse(QApplication::arguments()))
    {
#ifdef Q_OS_WIN
        QMessageBox::critical(nullptr, "Procyon", parser.errorText());
#else
        qCritical() << qPrintable(parser.errorText());
#endif
        return false;
    }

    // These will quite the app
    if (parser.isSet(optionHelp))
        parser.showHelp();
    if (parser.isSet(optionVersion))
        parser.showVersion();

    // It's only useful on Windows where there is no
    // direct way to use console from GUI applications.
    if (parser.isSet(optionConsole))
        Ori::Debug::installMessageHandler();

    AppSettings::instance().isDevMode = parser.isSet(optionDevMode);

    return true;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Procyon");
    app.setOrganizationName("orion-project.org");
    app.setApplicationVersion(APP_VER);
    app.setStyle(QStyleFactory::create("Fusion"));

    if (!processCommandLine()) return 1;

    // Load settings
    auto s1 = Ori::Settings::open();
    AppSettings::instance().load(s1);

    // Call `setStyleSheet` after setting loaded
    // to be able to apply custom colors.
    app.setStyleSheet(loadStyleSheet(s1));

    MainWindow  w;
    w.loadSettings(s1);
    delete s1;

    w.show();
    int res = app.exec();

    // Save settings
    auto s2 = Ori::Settings::open();
    AppSettings::instance().save(s2);
    w.saveSettings(s2);
    delete s2;

    return res;
}
