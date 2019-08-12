#include "MainWindow.h"

#include "AppSettings.h"

#include "tools/OriDebug.h"
#include "tools/OriSettings.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QStyleFactory>
#include <QCommandLineParser>
#include <QMessageBox>

QString loadStyleSheet(QSettings* s)
{
    QFile file(":/style/app_main");
    bool ok = file.open(QIODevice::ReadOnly);
    if (!ok)
    {
        qWarning() << "Unable to load style from resources" << file.errorString();
        return QString();
    }
    QByteArray data = file.readAll();
    if (data.isEmpty())
    {
        qWarning() << "Unable to load style from resources: read data is empty";
        return QString();
    }

    QString styleSheet(data);

    Ori::SettingsGroup group(s, "Theme");

    // General window color, it can be set slightly different than the default, for example,
    // to make it better match the window borders color depending on the desktop theme.
    styleSheet.replace("$base-color", s->value("baseColor", "#dadbde").toString());

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
        QMessageBox::critical(nullptr, app.applicationName(), parser.errorText());
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

    Settings::instance().isDevMode = parser.isSet(optionDevMode);

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

    MainWindow w;

    { // Settings scope
        QSharedPointer<QSettings> s(Ori::Settings::open());

        Settings::instance().load(s.data());

        // Call `setStyleSheet` after setting loaded
        // to be able to apply custom colors.
        app.setStyleSheet(loadStyleSheet(s.data()));

        w.loadSettings(s.data());
        w.show();
    }

    int res = app.exec();

    { // Settings scope
        QSharedPointer<QSettings> s(Ori::Settings::open());

        Settings::instance().save(s.data());

        w.saveSettings(s.data());
    }

    return res;
}
