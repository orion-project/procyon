#include "MemoManager.h"

#include "Catalog.h"
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
    const QString type = "Type";
    const QString data = "Data";
    const QString created = "Created";
    const QString updated = "Updated";
    const QString station = "Station";

    QString sqlCreate() const override {
        return "CREATE TABLE IF NOT EXISTS Memo ("
               "Id INTEGER PRIMARY KEY, "
               "Parent REFERENCES Folder(Id) ON DELETE CASCADE, "
               "Title, Type, Data, Created, Updated, Station)";
    }

    const QString sqlSelectAllNoData =
        "SELECT Id, Parent, Title, Type, Created, Updated, Station FROM Memo";

    virtual QString sqlSelectDataById(int id) const {
        return QString("SELECT Data FROM Memo WHERE Id = %1").arg(id);
    }

    const QString sqlInsert =
        "INSERT INTO Memo (Id, Parent, Title, Type, Data, Created, Updated, Station) "
        "VALUES (:Id, :Parent, :Title, :Type, :Data, :Created, :Updated, :Station)";

    const QString sqlUpdate =
        "UPDATE Memo SET Title = :Title, Data = :Data, Updated = :Updated, Station = :Station "
        "WHERE Id = :Id";

    const QString sqlDelete = "DELETE FROM Memo WHERE Id = :Id";
};

class MemoOptionsTableDef : public Ori::Sql::TableDef
{
public:
    MemoOptionsTableDef() : Ori::Sql::TableDef("MemoOptions") {}

    const QString memoId = "MemoId";
    const QString name = "Name";
    const QString value = "Value";

    QString sqlCreate() const override {
        return "CREATE TABLE IF NOT EXISTS MemoOptions ("
               "MemoId REFERENCES Memo(Id) ON DELETE CASCADE, "
               "Name, Value)";
    }

    const QString sqlSelect(int memoId) const {
        return QString("SELECT Name, Value from MemoOptions WHERE MemoId = %1").arg(memoId);
    }

    const QString sqlUpdate =
        "REPLACE INTO MemoOptions (MemoId, Name, Value) VALUES (:MemoId, :Name, :Value)";
};

MemoTableDef* memoTable() { static MemoTableDef t; return &t; }
MemoOptionsTableDef* memoOptionsTable() { static MemoOptionsTableDef t; return &t; }

} // namespace

//------------------------------------------------------------------------------
//                               MemoManager
//------------------------------------------------------------------------------

QString MemoManager::prepare()
{
    QString res = createTable(memoTable());
    if (!res.isEmpty()) return res;

    return createTable(memoOptionsTable());
}

QString MemoManager::create(MemoItem* item) const
{
    auto table = memoTable();

    SelectQuery queryId(table->sqlSelectMaxId());
    if (queryId.isFailed() || !queryId.next())
        return QString("Unable to generate id for new memo.\n\n%1").arg(queryId.error());

    int newId = queryId.record().value(0).toInt() + 1;
    item->_id = newId;

    auto res = ActionQuery(table->sqlInsert)
            .param(table->parent, item->parent() ? item->parent()->asFolder()->id() : 0)
            .param(table->id, item->id())
            .param(table->title, item->title())
            .param(table->type, item->type()->name())
            .param(table->data, item->data())
            .param(table->created, item->created())
            .param(table->updated, item->updated())
            .param(table->station, item->station())
            .exec();
    if (!res.isEmpty())
        return QString("Failed to create new memo.\n\n%1").arg(res);

    return QString();
}

MemosResult MemoManager::selectAll() const
{
    auto table = memoTable();

    MemosResult result;

    SelectQuery query(table->sqlSelectAllNoData);
    if (query.isFailed())
    {
        result.error = QString("Unable to load memos.\n\n%1").arg(query.error());
        return result;
    }

    while (query.next())
    {
        auto r = query.record();

        MemoItem *item = new MemoItem;
        item->_id = r.value(table->id).toInt();
        item->_title = r.value(table->title).toString();
        item->_type = getMemoType(r.value(table->type).toString());
        item->_created = r.value(table->created).toDateTime();
        item->_updated = r.value(table->updated).toDateTime();
        item->_station = r.value(table->station).toString();

        int parentId = r.value(table->parent).toInt();
        if (!result.items.contains(parentId))
            result.items.insert(parentId, QList<MemoItem*>());
        static_cast<QList<MemoItem*>&>(result.items[parentId]).append(item);
        result.allMemos.insert(item->id(), item);
    }

    return result;
}

QString MemoManager::load(MemoItem* memo) const
{
    auto table = memoTable();

    SelectQuery query(table->sqlSelectDataById(memo->id()));
    if (query.isFailed())
        return QString("Unable to load memo #%1.\n\n%2").arg(memo->id()).arg(query.error());

    if (!query.next())
        return QString("Memo #%1 does not exist.").arg(memo->id());

    QSqlRecord r = query.record();
    memo->_data = r.value(table->data).toString();
    memo->_isLoaded = true;
    return QString();
}

QString MemoManager::update(MemoItem* memo, const MemoUpdateParam& update) const
{
    auto table = memoTable();
    return ActionQuery(table->sqlUpdate)
            .param(table->id, memo->id())
            .param(table->title, update.title)
            .param(table->data, update.data)
            .param(table->updated, update.moment)
            .param(table->station, update.station)
            .exec();
}

QString MemoManager::remove(MemoItem* item) const
{
    auto table = memoTable();
    return ActionQuery(table->sqlDelete)
            .param(table->id, item->id())
            .exec();
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

QMap<QString, QVariant> MemoManager::selectOptions(int memoId) const
{
    QMap<QString, QVariant> options;
    auto table = memoOptionsTable();

    SelectQuery query(table->sqlSelect(memoId));
    if (query.isFailed())
    {
        qWarning() << "Unable to select options for memo" << memoId << query.error();
        return options;
    }

    while (query.next())
    {
        auto r = query.record();
        options[r.value(table->name).toString()] = r.value(table->value);
    }

    return options;
}

QString MemoManager::updateOption(int memoId, const QString& name, const QVariant& value) const
{
    auto table = memoOptionsTable();
    return ActionQuery(table->sqlUpdate)
            .param(table->memoId, memoId)
            .param(table->name, name)
            .param(table->value, value)
            .exec();
}
