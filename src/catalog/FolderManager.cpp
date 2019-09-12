#include "FolderManager.h"

#include "Catalog.h"
#include "SqlHelper.h"

using namespace Ori::Sql;

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
    const QString info = "Info";

    QString sqlCreate() const override {
        return "CREATE TABLE IF NOT EXISTS Folder ("
               "Id INTEGER PRIMARY KEY, "
               "Parent, Title, Info)";
    }

    const QString sqlInsert =
        "INSERT INTO Folder (Id, Parent, Title, Info) "
        "VALUES (:Id, :Parent, :Title, :Info)";

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
                .param(table->parent, folder->parent() ? folder->parent()->asFolder()->id() : 0)
                .param(table->title, folder->title())
                .param(table->info, folder->info())
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
        int id = r.value(table->id).toInt();
        if (!result.items.contains(id))
            result.items.insert(id, new FolderItem);
        auto item = result.items[id];
        item->_id = id;
        item->_title = r.value(table->title).toString();
        item->_info = r.value(table->info).toString();
        int parentId = r.value(table->parent).toInt();
        if (parentId > 0)
        {
            if (!result.items.contains(parentId))
                result.items.insert(parentId, new FolderItem);
            auto parentItem = result.items[parentId];
            parentItem->_children.append(item);
            item->_parent = parentItem;
        }
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

    for (auto item: folder->children())
        if (item->isFolder())
        {
            QString res = removeBranch(item->asFolder(), thisPath);
            if (!res.isEmpty()) return res;
        }

    QString res = ActionQuery(table->sqlDelete)
            .param(table->id, folder->id())
            .exec();
    if (!res.isEmpty())
        return QString("Failed to delete folder '%1'.\n\n%2").arg(thisPath).arg(res);
    return QString();
}
