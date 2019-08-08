#include "MainWindow.h"

#include "AppSettings.h"

#include "tools/OriDebug.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QStyleFactory>
#include <QCommandLineParser>
#include <QMessageBox>

#ifndef Q_OS_WIN
#include <iostream>
#endif

QString loadStyleSheet()
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
    return data;
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
    app.setStyleSheet(loadStyleSheet());

    Settings::instance().load();

    if (!processCommandLine()) return 1;

    MainWindow w;
    w.show();

    return app.exec();
}
