#include "MemoStore.h"

#include "Enot.h"
#include "MemoType.h"
#include "SqlHelper.h"

using namespace Ori::Sql;

namespace Store
{
MemoStore* memos() { static MemoStore s; return &s; }
}

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
//                               MemoStore
//------------------------------------------------------------------------------

QString MemoStore::prepare()
{
    auto table = memoTable();

    QString res = createTable(table);
    if (!res.isEmpty()) return res;

    res = addColumnIfNotExist(table->tableName(), table->updated);
    if (!res.isEmpty()) return res;

    res = addColumnIfNotExist(table->tableName(), table->created);
    if (!res.isEmpty()) return res;

    res = addColumnIfNotExist(table->tableName(), table->station);
    if (!res.isEmpty()) return res;

    return createTable(memoOptionsTable());
}

QString MemoStore::create(Memo* memo) const
{
    auto table = memoTable();

    SelectQuery queryId(table->sqlSelectMaxId());
    if (queryId.isFailed() || !queryId.next())
        return QString("Unable to generate id for new memo.\n\n%1").arg(queryId.error());

    int newId = queryId.record().value(0).toInt() + 1;
    memo->_id = newId;

    auto res = ActionQuery(table->sqlInsert)
            .param(table->parent, memo->parent() ? memo->parent()->asFolder()->id() : 0)
            .param(table->id, memo->id())
            .param(table->title, memo->title())
            .param(table->type, memo->type()->name())
            .param(table->data, memo->data())
            .param(table->created, memo->created())
            .param(table->updated, memo->updated())
            .param(table->station, memo->station())
            .exec();
    if (!res.isEmpty())
        return QString("Failed to create new memo.\n\n%1").arg(res);

    return QString();
}

MemosResult MemoStore::selectAll() const
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

        Memo *memo = new Memo;
        memo->_id = r.value(table->id).toInt();
        memo->_title = r.value(table->title).toString();
        memo->_type = MemoType::findByName(r.value(table->type).toString());
        memo->_created = r.value(table->created).toDateTime();
        memo->_updated = r.value(table->updated).toDateTime();
        memo->_station = r.value(table->station).toString();

        int folderId = r.value(table->parent).toInt();
        result.items.append({folderId, memo});
    }

    return result;
}

QString MemoStore::load(Memo* memo) const
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

QString MemoStore::update(Memo* memo, const MemoUpdateParam& update) const
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

QString MemoStore::remove(Memo* memo) const
{
    auto table = memoTable();
    return ActionQuery(table->sqlDelete)
            .param(table->id, memo->id())
            .exec();
}

QString MemoStore::countAll(int *count) const
{
    auto table = memoTable();
    SelectQuery query(table->sqlCountAll());
    if (query.isFailed()) return query.error();

    query.next();
    *count = query.record().value(0).toInt();
    return QString();
}

QMap<QString, QVariant> MemoStore::selectOptions(int memoId) const
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

QString MemoStore::updateOption(int memoId, const QString& name, const QVariant& value) const
{
    auto table = memoOptionsTable();
    return ActionQuery(table->sqlUpdate)
            .param(table->memoId, memoId)
            .param(table->name, name)
            .param(table->value, value)
            .exec();
}
