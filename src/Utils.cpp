#include "Utils.h"

#include <QDebug>
#include <QFile>

QString loadTextFromResource(const QString& fileName)
{
    QFile file(fileName);
    bool ok = file.open(QIODevice::ReadOnly);
    if (!ok)
    {
        qWarning() << "Unable to open resource file" << fileName << file.errorString();
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}
