#include "MemoManager.h"

#include "Catalog.h"
#include "Memo.h"
#include "SqlHelper.h"

using namespace Ori::Sql;

//------------------------------------------------------------------------------
//                               MemoTableDef
//------------------------------------------------------------------------------

namespace {

class MemoTableDef : public Ori::Sql::TableDef
{
public:
    MemoTableDef() : Ori::Sql::TableDef("Memo") {}

    const QString id = "Id";
    const QString parent = "Parent";
    const QString title = "Title";
    const QString info = "Info";
    const QString type = "Type";
    const QString data = "Data";

    QString sqlCreate() const override {
        return "CREATE TABLE IF NOT EXISTS Memo ("
               "Id INTEGER PRIMARY KEY, "
               "Parent REFERENCES Folder(Id) ON DELETE CASCADE, "
               "Title, Info, Type, Data)";
    }

    const QString sqlInsert =
        "INSERT INTO Memo (Id, Parent, Title, Info, Type, Data) "
        "VALUES (:Id, :Parent, :Title, :Info, :Type, :Data)";

    const QString sqlUpdate =
        "UPDATE Memo SET Title = :Title, Info = :Info, Type = :Type, Data = :Data "
        "WHERE Id = :Id";

    const QString sqlDelete = "DELETE FROM Memo WHERE Id = :Id";
};

MemoTableDef* memoTable() { static MemoTableDef t; return &t; }

} // namespace

//------------------------------------------------------------------------------
//                               MemoManager
//------------------------------------------------------------------------------

QString MemoManager::prepare()
{
    return createTable(memoTable());
}

QString MemoManager::create(MemoItem* item) const
{
    auto table = memoTable();

    SelectQuery queryId(table->sqlSelectMaxId());
    if (queryId.isFailed() || !queryId.next())
        return QString("Unable to generate id for new memo.\n\n%1").arg(queryId.error());

    if (!item->memo() || !item->type())
        return "Bad method usage: poorly defined memo: no content or type is defined";

    int newId = queryId.record().value(0).toInt() + 1;
    item->_id = newId;
    item->memo()->_id = newId;

    auto res = ActionQuery(table->sqlInsert)
            .param(table->parent, item->parent() ? item->parent()->asFolder()->id() : 0)
            .param(table->id, item->memo()->id())
            .param(table->title, item->memo()->title())
            .param(table->info, item->info())
            .param(table->type, item->memo()->type()->name())
            .param(table->data, item->memo()->data())
            .exec();
    if (!res.isEmpty())
        return QString("Failed to create new memo.\n\n%1").arg(res);

    return QString();
}

MemosResult MemoManager::selectAll() const
{
    auto table = memoTable();

    MemosResult result;

    SelectQuery query(table->sqlSelectAll());
    if (query.isFailed())
    {
        result.error = QString("Unable to load memos.\n\n%1").arg(query.error());
        return result;
    }

    while (query.next())
    {
        auto r = query.record();

        int id = r.value(table->id).toInt();
        QString title = r.value(table->title).toString();
        QString memoType = r.value(table->type).toString();
        if (!memoTypes().contains(memoType))
        {
            result.warnings.append(
                QString("Skip memo #%1 '%2': unknown memo type '%3'.")
                    .arg(id).arg(title).arg(memoType));
            continue;
        }

        MemoItem *item = new MemoItem;
        item->_id = id;
        item->_title = title;
        item->_info = r.value(table->info).toString();
        item->_type = memoTypes()[memoType];

        int parentId = r.value(table->parent).toInt();
        if (!result.items.contains(parentId))
            result.items.insert(parentId, QList<MemoItem*>());
        static_cast<QList<MemoItem*>&>(result.items[parentId]).append(item);
        result.allMemos.insert(id, item);
    }

    return result;
}

QString MemoManager::load(Memo* memo) const
{
    auto table = memoTable();

    SelectQuery query(table->sqlSelectById(memo->id()));
    if (query.isFailed())
        return QString("Unable to load memo #%1.\n\n%2").arg(memo->id()).arg(query.error());

    if (!query.next())
        return QString("Memo #%1 does not exist.").arg(memo->id());

    QSqlRecord r = query.record();
    memo->_title = r.value(table->title).toString();
    memo->_data = r.value(table->data).toString();
    return QString();
}

QString MemoManager::update(Memo* memo, const QString &info) const
{
    auto table = memoTable();
    return ActionQuery(table->sqlUpdate)
            .param(table->id, memo->id())
            .param(table->title, memo->title())
            .param(table->info, info)
            .param(table->type, memo->type()->name())
            .param(table->data, memo->data())
            .exec();
}

QString MemoManager::remove(MemoItem* item) const
{
    auto table = memoTable();
    return ActionQuery(table->sqlDelete)
            .param(table->id, item->id())
            .exec();
    // TODO remove memo specific data
}

QString MemoManager::countAll(int *count) const
{
    auto table = memoTable();
    SelectQuery query(table->sqlCountAll());
    if (query.isFailed()) return query.error();

    query.next();
    *count = query.record().value(0).toInt();
    return QString();
}
