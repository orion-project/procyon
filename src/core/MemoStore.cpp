#include "MemoStore.h"

#include "Enot.h"
#include "MemoType.h"
#include "SqlHelper.h"

using namespace Ori::Sql;
using namespace Qt::StringLiterals;

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
        return u"CREATE TABLE IF NOT EXISTS Memo ("
               "Id INTEGER PRIMARY KEY, "
               "Parent INTEGER NOT NULL, "
               "Title TEXT, "
               "Type TEXT, "
               "Data TEXT, "
               "Created DATETIME DEFAULT CURRENT_TIMESTAMP, "
               "Updated DATETIME DEFAULT CURRENT_TIMESTAMP, "
               "Station TEXT,"
               "FOREIGN KEY (Parent) REFERENCES Folder(Id) ON DELETE CASCADE"
               ")"_s;
    }

    const QString sqlSelectAllNoData =
        u"SELECT Id, Parent, Title, Type, Created, Updated, Station FROM Memo"_s;

    virtual QString sqlSelectDataById(int id) const {
        return QString("SELECT Data FROM Memo WHERE Id = %1").arg(id);
    }

    const QString sqlInsert =
        u"INSERT INTO Memo (Id, Parent, Title, Type, Data, Created, Updated, Station) "
        "VALUES (:Id, :Parent, :Title, :Type, :Data, :Created, :Updated, :Station)"_s;

    const QString sqlDelete = u"DELETE FROM Memo WHERE Id = :Id"_s;
};

struct MemoOptionsTable
{
    inline static const auto& tableName = u"MemoOptions"_s;

    struct C
    {
        inline static const auto& memoId = u"MemoId"_s;
        inline static const auto& name = u"Name"_s;
        inline static const auto& value = u"Value"_s;
    };

    inline static const auto& sqlCreate =
        u"CREATE TABLE IF NOT EXISTS MemoOptions ("
        "MemoId INTEGER NOT NULL, "
        "Name TEXT NOT NULL, "
        "Value TEXT, "
        "FOREIGN KEY (MemoId) REFERENCES Memo(Id) ON DELETE CASCADE, "
        "UNIQUE(MemoId, Name))"_s;

    inline static const auto& sqlSelect =
        u"SELECT Name, Value from MemoOptions WHERE MemoId = :MemoId"_s;

    inline static const auto& sqlUpdate =
        u"REPLACE INTO MemoOptions (MemoId, Name, Value) VALUES (:MemoId, :Name, :Value)"
        "ON CONFLICT (MemoId, Name) DO UPDATE SET Value = :Value"_s;
};

struct MemoPropsTable
{
    inline static const auto& tableName = u"MemoProps"_s;

    struct C
    {
        inline static const auto& memoId = u"MemoId"_s;
        inline static const auto& name = u"Name"_s;
        inline static const auto& value = u"Value"_s;
    };

    inline static const auto& sqlCreate =
        u"CREATE TABLE IF NOT EXISTS MemoProps ("
        "MemoId INTEGER NOT NULL, "
        "Name TEXT NOT NULL, "
        "Value TEXT, "
        "FOREIGN KEY (MemoId) REFERENCES Memo(Id) ON DELETE CASCADE, "
        "UNIQUE(MemoId, Name))"_s;

    inline static const auto& sqlSelect =
        u"SELECT Name, Value from MemoProps WHERE MemoId = :MemoId"_s;

    inline static const auto& sqlUpdate =
        u"INSERT INTO MemoProps (MemoId, Name, Value) VALUES (:MemoId, :Name, :Value) "
        "ON CONFLICT (MemoId, Name) DO UPDATE SET Value = :Value"_s;

    inline static const auto& sqlDelete =
        u"DELETE FROM MemoProps WHERE MemoId = :MemoId AND Name = :Name"_s;

    inline static const auto& sqlSelectNames =
        u"SELECT DISTINCT Name from MemoProps ORDER BY Name"_s;

    inline static const auto& sqlSelectValues =
        u"SELECT DISTINCT Value from MemoProps WHERE Name = :Name ORDER BY Value"_s;
};

struct MemoLinksTable
{
    inline static const auto& tableName = u"MemoLinks"_s;

    struct C
    {
        inline static const auto& id1 = u"Id1"_s;
        inline static const auto& id2 = u"Id2"_s;
        inline static const auto& created = u"Created"_s;
        inline static const auto& station = u"Station"_s;
    };

    inline static const auto& sqlCreate =
        u"CREATE TABLE IF NOT EXISTS MemoLinks ("
        "Id1 INTEGER NOT NULL, "
        "Id2 INTEGER NOT NULL, "
        "Created DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "Station TEXT, "
        "PRIMARY KEY(Id1, Id2),"
        "FOREIGN KEY (Id1) REFERENCES Memo(Id) ON DELETE CASCADE, "
        "FOREIGN KEY (Id2) REFERENCES Memo(Id) ON DELETE CASCADE)"_s;
};

struct MemoHistoryTable
{
    inline static const auto& tableName = u"MemoHistory"_s;

    inline static const auto& sqlCreate =
        u"CREATE TABLE IF NOT EXISTS MemoHistory ("
        "MemoId INTEGER NOT NULL, "
        "What TEXT, "
        "Value TEXT, "
        "Moment DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "Station TEXT, "
        "FOREIGN KEY (MemoId) REFERENCES Memo(Id) ON DELETE CASCADE)"_s;
};

struct MemoSheetsTable
{
    inline static const auto& tableName = u"MemoSheets"_s;

    inline static const auto& sqlCreate =
        u"CREATE TABLE IF NOT EXISTS MemoSheets ("
        "Id INTEGER PRIMARY KEY, "
        "MemoId INTEGER NOT NULL, "
        "Data TEXT, "
        "Created DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "Updated DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "Station TEXT, "
        "FOREIGN KEY (MemoId) REFERENCES Memo(Id) ON DELETE CASCADE)"_s;
};

MemoTableDef* memoTable() { static MemoTableDef t; return &t; }

} // namespace

//------------------------------------------------------------------------------
//                               MemoStore
//------------------------------------------------------------------------------

QString MemoStore::prepare()
{
    auto table = memoTable();

    QString res = createTable(table);
    if (!res.isEmpty()) return res;

    res = maybeAddColumn(table->tableName(), table->updated);
    if (!res.isEmpty()) return res;

    res = maybeAddColumn(table->tableName(), table->created);
    if (!res.isEmpty()) return res;

    res = maybeAddColumn(table->tableName(), table->station);
    if (!res.isEmpty()) return res;

    res = maybeAddIndex(table->tableName(), table->parent);

    {
        using T = MemoOptionsTable;

        res = createTable<T>();
        if (!res.isEmpty()) return res;

        res = maybeAddConstrain(T::tableName, {T::C::memoId, T::C::name});
        if (!res.isEmpty()) return res;
    }

    res = createTable<MemoPropsTable>();
    if (!res.isEmpty()) return res;
    
    {
        using T = MemoLinksTable;

        res = createTable<T>();
        if (!res.isEmpty()) return res;

        res = maybeAddIndex(T::tableName, T::C::id2);
        if (!res.isEmpty()) return res;
    }

    res = createTable<MemoHistoryTable>();
    if (!res.isEmpty()) return res;

    res = createTable<MemoSheetsTable>();
    if (!res.isEmpty()) return res;

    return {};
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
            .param(table->parent, memo->parent() ? memo->parent()->id() : 0)
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
        const auto& r = query.record();

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

    const auto& r = query.record();
    memo->_data = r.value(table->data).toString();
    memo->_isLoaded = true;
    return QString();
}

QString MemoStore::update(Memo* memo, const MemoUpdateParam& update) const
{
    auto table = memoTable();

    QStringList sql;
    sql << u"UPDATE Memo SET"_s;
    if (update.title)
        sql << u"Title = :Title,"_s;
    if (update.data)
        sql << u"Data = :Data,"_s;
    if (update.station)
        sql << u"Station = :Station,"_s;
    sql << u"Updated = :Updated WHERE Id = :Id"_s;

    auto q = AnyQuery(sql.join(' ')).param(table->id, memo->id());
    if (update.title)
        q.param(table->title, *update.title);
    if (update.data)
        q.param(table->data, *update.data);
    if (update.station)
        q.param(table->station, *update.station);
    if (update.moment)
        q.param(table->updated, *update.moment);
    else
        q.param(table->updated, QDateTime::currentDateTime());

    q.exec();
    return q.error();
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

QHash<QString, QVariant> MemoStore::selectOptions(int memoId) const
{
    using T = MemoOptionsTable;

    auto q = AnyQuery(T::sqlSelect).param(T::C::memoId, memoId).exec();
    if (q.isFailed())
    {
        qWarning() << "Unable to select options for memo" << memoId << q.error();
        return {};
    }

    QHash<QString, QVariant> options;
    while (q.next())
        options.insert(q.valueStr(T::C::name), q.valueStr(T::C::value));
    return options;
}

QString MemoStore::updateOption(int memoId, const QString& name, const QVariant& value) const
{
    using T = MemoOptionsTable;
    return AnyQuery(T::sqlUpdate)
            .param(T::C::memoId, memoId)
            .param(T::C::name, name)
            .param(T::C::value, value)
            .exec()
            .error();
}

QHash<QString, QString> MemoStore::loadProps(int memoId) const
{
    using T = MemoPropsTable;

    auto q = AnyQuery(T::sqlSelect).param(T::C::memoId, memoId).exec();
    if (q.isFailed())
    {
        // TODO: add protocol
        qWarning() << "Unable to load props for memo" << memoId << q.error();
        return {};
    }

    QHash<QString, QString> result;
    while (q.next())
        result.insert(q.valueStr(T::C::name), q.valueStr(T::C::value));
    return result;
}

QStringList MemoStore::loadPropNames() const
{
    using T = MemoPropsTable;

    auto q = AnyQuery(T::sqlSelectNames).exec();
    if (q.isFailed())
    {
        // TODO: add protocol
        qWarning() << "Unable to load memo props names" << q.error();
        return {};
    }

    QStringList result;
    while (q.next())
        result.append(q.valueStr(T::C::name));
    return result;
}

QStringList MemoStore::loadPropValues(const QString& name) const
{
    using T = MemoPropsTable;

    auto q = AnyQuery(T::sqlSelectValues).param(T::C::name, name).exec();
    if (q.isFailed())
    {
        // TODO: add protocol
        qWarning() << "Unable to load values for prop" << name << q.error();
        return {};
    }

    QStringList result;
    while (q.next())
        result.append(q.valueStr(T::C::value));
    return result;
}

QString MemoStore::deleteProp(int memoId, const QString& name) const
{
    using T = MemoPropsTable;
    return AnyQuery(T::sqlDelete)
        .param(T::C::memoId, memoId)
        .param(T::C::name, name)
        .exec()
        .error();
}

QString MemoStore::updateProp(int memoId, const QString& name, const QString& value) const
{
    using T = MemoPropsTable;
    return AnyQuery(T::sqlUpdate)
        .param(T::C::memoId, memoId)
        .param(T::C::name, name)
        .param(T::C::value, value)
        .exec()
        .error();
}
