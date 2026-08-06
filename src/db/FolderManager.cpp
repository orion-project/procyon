#include "FolderManager.h"

#include "Db.h"
#include "SqlHelper.h"

using namespace Ori::Sql;

namespace DB
{
FolderManager *folderManager() { static FolderManager m; return &m; }
}

//------------------------------------------------------------------------------
//                                FolderTableDef
//------------------------------------------------------------------------------

namespace  {

class FolderTableDef : public TableDef
{
public:
    FolderTableDef() : TableDef("Folder") {}

    const QString id = "Id";
    const QString parent = "Parent";
    const QString title = "Title";

    QString sqlCreate() const override {
        return "CREATE TABLE IF NOT EXISTS Folder ("
               "Id INTEGER PRIMARY KEY, "
               "Parent, Title)";
    }

    const QString sqlInsert =
        "INSERT INTO Folder (Id, Parent, Title) "
        "VALUES (:Id, :Parent, :Title)";

    const QString sqlRename = "UPDATE Folder SET Title = :Title WHERE Id = :Id";
    const QString sqlDelete = "DELETE FROM Folder WHERE Id = :Id";
};

FolderTableDef* folderTable() { static FolderTableDef t; return &t; }

} // namespace

//------------------------------------------------------------------------------
//                                FolderManager
//------------------------------------------------------------------------------

QString FolderManager::prepare()
{
    return createTable(folderTable());
}

QString FolderManager::create(FolderItem* folder) const
{
    auto table = folderTable();

    SelectQuery queryId(table->sqlSelectMaxId());
    if (queryId.isFailed() || !queryId.next())
        return qApp->tr("Unable to generate id for new folder.\n\n%1").arg(queryId.error());

    folder->_id = queryId.record().value(0).toInt() + 1;

    auto res = ActionQuery(table->sqlInsert)
                .param(table->id, folder->id())
                .param(table->parent, folder->parent()->id())
                .param(table->title, folder->title())
                .exec();
    if (!res.isEmpty())
        return qApp->tr("Failed to create new folder.\n\n%1").arg(res);

    return QString();
}

FoldersResult FolderManager::selectAll() const
{
    FoldersResult result;

    auto table = folderTable();

    SelectQuery query(table->sqlSelectAll());
    if (query.isFailed())
    {
        result.error = qApp->tr("Unable to load folder list.\n\n%1").arg(query.error());
        return result;
    }

    while (query.next())
    {
        auto r = query.record();
        auto folder = new FolderItem;
        folder->_id = r.value(table->id).toInt();
        folder->_title = r.value(table->title).toString();
        int parentId = r.value(table->parent).toInt();
        result.items.append({parentId, folder});
    }

    return result;
}

QString FolderManager::rename(int folderId, const QString title) const
{
    auto table = folderTable();
    return ActionQuery(table->sqlRename)
            .param(table->id, folderId)
            .param(table->title, title)
            .exec();
}

QString FolderManager::remove(FolderItem *folder) const
{
    auto db = QSqlDatabase::database();
    bool ok = db.transaction();
    if (!ok)
        return QString("Unable to start transaction for removing folder #%1.\n\n%2")
                .arg(folder->id()).arg(SqlHelper::errorText(db.lastError()));

    QString res = removeBranch(folder, QString());
    if (!res.isEmpty())
    {
        db.rollback();
        return res;
    }

    db.commit();
    return QString();
}

QString FolderManager::removeBranch(FolderItem* folder, const QString& path) const
{
    auto table = folderTable();
    QString thisPath = path + '/' + folder->title();

    for (auto item: folder->folders())
    {
        QString res = removeBranch(item->asFolder(), thisPath);
        if (!res.isEmpty()) return res;
    }

    QString res = ActionQuery(table->sqlDelete)
            .param(table->id, folder->id())
            .exec();
    if (!res.isEmpty())
        return QString("Failed to delete folder '%1'.\n\n%2").arg(thisPath, res);

    // Memos are DB deleted by FK relation

    return QString();
}
