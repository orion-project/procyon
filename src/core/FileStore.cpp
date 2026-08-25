#include "FileStore.h"

#include <QDir>
#include <QSqlDatabase>

namespace Store
{
FileStore* files() { static FileStore s; return &s; }
}

//------------------------------------------------------------------------------
//                               Helpers
//------------------------------------------------------------------------------

static const QStringList& supportedImgExts()
{
    static const QStringList suffixes({".png", ".jpg", ".jpeg"});
    return suffixes;
}

//------------------------------------------------------------------------------
//                               FileStore
//------------------------------------------------------------------------------

bool FileStore::isSupportedImg(const QString& path)
{
    for (const auto& suffix : supportedImgExts())
        if (path.endsWith(suffix, Qt::CaseInsensitive))
            return true;
    return false;
}

QFileInfo FileStore::attachedFile(const QString& fileName)
{
    QFileInfo file(QSqlDatabase::database().databaseName());
    file.setFile(file.absoluteDir().path() + '/' + file.completeBaseName() +
                 QStringLiteral(".files/") + fileName);
    return file;
}
