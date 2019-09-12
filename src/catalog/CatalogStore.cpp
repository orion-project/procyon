#include "CatalogStore.h"

#include "SqlHelper.h"

namespace CatalogStore {

MemoManager* memoManager() { static MemoManager m; return &m; }
FolderManager *folderManager() { static FolderManager m; return &m; }
SettingsManager* settingsManager() { static SettingsManager m; return &m; }

QString openDatabase(const QString fileName)
{
    auto db = QSqlDatabase::database();

    if (!db.isValid())
        db = QSqlDatabase::addDatabase("QSQLITE");

    if (db.isOpen())
        db.close();

    db.setDatabaseName(fileName);

    if (!db.open())
        return QString("Unable to open database connection.\n\n%1")
                .arg(SqlHelper::errorText(db.lastError()));

    QSqlQuery query;
    if (!query.exec("PRAGMA foreign_keys = ON;"))
        return QString("Failed to enable foreign keys.\n\n%1")
                .arg(SqlHelper::errorText(query));

    bool ok = db.transaction();
    if (!ok)
        return QString("Failed to begin transaction for setup database structure.\n\n%1")
                .arg(SqlHelper::errorText(db.lastError()));

    QString res;

    res = folderManager()->prepare();
    if (!res.isEmpty()) return res;

    res = memoManager()->prepare();
    if (!res.isEmpty()) return res;

    res = settingsManager()->prepare();
    if (!res.isEmpty()) return res;

    db.commit();
    return QString();
}

} // namespace CatalogStore
