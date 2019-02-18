#include "MainWindow.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QStyleFactory>

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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Procyon");
    app.setOrganizationName("orion-project.org");
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setStyleSheet(loadStyleSheet());

    MainWindow w;
    w.show();

    return app.exec();
}
