#include "Catalog.h"
#include "CatalogStore.h"
#include "Memo.h"
#include "SqlHelper.h"

#include <QDebug>

using namespace Ori::Sql;

// TODO check if strings from this file are put in language file

//------------------------------------------------------------------------------

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

//------------------------------------------------------------------------------

class MemoTableDef : public TableDef
{
public:
    MemoTableDef() : TableDef("Memo") {}

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

//------------------------------------------------------------------------------

class SettingsTableDef : public TableDef
{
public:
    SettingsTableDef() : TableDef("Settings") {}

    const QString id = "Id";
    const QString value = "Value";

    QString sqlCreate() const override {
        return "CREATE TABLE IF NOT EXISTS Settings (Id, Value)";
    }

    const QString sqlInsert = "INSERT INTO Settings (Id, Value) VALUES (:Id, :Value)";
    const QString sqlUpdate = "UPDATE Settings SET Value = :Value WHERE Id = :Id";
};

//------------------------------------------------------------------------------

FolderTableDef* FolderManager::table() const { static FolderTableDef t; return &t; }

QString FolderManager::create(FolderItem* folder) const
{
    SelectQuery queryId(table()->sqlSelectMaxId());
    if (queryId.isFailed() || !queryId.next())
        return qApp->tr("Unable to generate id for new folder.\n\n%1").arg(queryId.error());

    folder->_id = queryId.record().value(0).toInt() + 1;

    auto res = ActionQuery(table()->sqlInsert)
                .param(table()->id, folder->id())
                .param(table()->parent, folder->parent() ? folder->parent()->asFolder()->id() : 0)
                .param(table()->title, folder->title())
                .param(table()->info, folder->info())
                .exec();
    if (!res.isEmpty())
        return qApp->tr("Failed to create new folder.\n\n%1").arg(res);

    return QString();
}

FoldersResult FolderManager::selectAll() const
{
    FoldersResult result;

    SelectQuery query(table()->sqlSelectAll());
    if (query.isFailed())
    {
        result.error = qApp->tr("Unable to load folder list.\n\n%1").arg(query.error());
        return result;
    }

    while (query.next())
    {
        auto r = query.record();
        int id = r.value(table()->id).toInt();
        if (!result.items.contains(id))
            result.items.insert(id, new FolderItem);
        auto item = result.items[id];
        item->_id = id;
        item->_title = r.value(table()->title).toString();
        item->_info = r.value(table()->info).toString();
        int parentId = r.value(table()->parent).toInt();
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
    return ActionQuery(table()->sqlRename)
            .param(table()->id, folderId)
            .param(table()->title, title)
            .exec();
}

QString FolderManager::remove(FolderItem *folder) const
{
    CatalogStore::beginTran(QString("Remove folder #%1").arg(folder->id()));
    QString res = removeBranch(folder, QString());
    if (!res.isEmpty())
    {
        CatalogStore::rollbackTran();
        return res;
    }
    CatalogStore::commitTran();
    return QString();
}

QString FolderManager::removeBranch(FolderItem* folder, const QString& path) const
{
    QString thisPath = path + '/' + folder->title();

    for (auto item: folder->children())
        if (item->isFolder())
        {
            QString res = removeBranch(item->asFolder(), thisPath);
            if (!res.isEmpty()) return res;
        }

    QString res = ActionQuery(table()->sqlDelete)
            .param(table()->id, folder->id())
            .exec();
    if (!res.isEmpty())
        return qApp->tr("Failed to delete folder '%1'.\n\n%2").arg(thisPath).arg(res);
    return QString();
}

//------------------------------------------------------------------------------

MemoTableDef* MemoManager::table() const { static MemoTableDef t; return &t; }

QString MemoManager::create(MemoItem* item) const
{
    SelectQuery queryId(table()->sqlSelectMaxId());
    if (queryId.isFailed() || !queryId.next())
        return qApp->tr("Unable to generate id for new memo.\n\n%1").arg(queryId.error());

    if (!item->memo() || !item->type())
        return "Bad method usage: poorly defined memo: no content or type is defined";

    int newId = queryId.record().value(0).toInt() + 1;
    item->_id = newId;
    item->memo()->_id = newId;

    auto res = ActionQuery(table()->sqlInsert)
            .param(table()->parent, item->parent() ? item->parent()->asFolder()->id() : 0)
            .param(table()->id, item->memo()->id())
            .param(table()->title, item->memo()->title())
            .param(table()->info, item->info())
            .param(table()->type, item->memo()->type()->name())
            .param(table()->data, item->memo()->data())
            .exec();
    if (!res.isEmpty())
        return qApp->tr("Failed to create new memo.\n\n%1").arg(res);

    return QString();
}

MemosResult MemoManager::selectAll() const
{
    MemosResult result;

    SelectQuery query(table()->sqlSelectAll());
    if (query.isFailed())
    {
        result.error = qApp->tr("Unable to load memos.\n\n%1").arg(query.error());
        return result;
    }

    while (query.next())
    {
        auto r = query.record();

        int id = r.value(table()->id).toInt();
        QString title = r.value(table()->title).toString();
        QString memoType = r.value(table()->type).toString();
        if (!memoTypes().contains(memoType))
        {
            result.warnings.append(qApp->tr("Skip memo #%1 '%2': "
                                            "unknown memo type '%3'.")
                                   .arg(id).arg(title).arg(memoType));
            continue;
        }

        MemoItem *item = new MemoItem;
        item->_id = id;
        item->_title = title;
        item->_info = r.value(table()->info).toString();
        item->_type = memoTypes()[memoType];

        int parentId = r.value(table()->parent).toInt();
        if (!result.items.contains(parentId))
            result.items.insert(parentId, QList<MemoItem*>());
        static_cast<QList<MemoItem*>&>(result.items[parentId]).append(item);
        result.allMemos.insert(id, item);
    }

    return result;
}

QString MemoManager::load(Memo* memo) const
{
    SelectQuery query(table()->sqlSelectById(memo->id()));
    if (query.isFailed())
        return qApp->tr("Unable to load memo #%1.\n\n%2").arg(memo->id()).arg(query.error());

    if (!query.next())
        return qApp->tr("Memo #%1 does not exist.").arg(memo->id());

    QSqlRecord r = query.record();
    memo->_title = r.value(table()->title).toString();
    memo->_data = r.value(table()->data).toString();
    return QString();
}

QString MemoManager::update(Memo* memo, const QString &info) const
{
    return ActionQuery(table()->sqlUpdate)
            .param(table()->id, memo->id())
            .param(table()->title, memo->title())
            .param(table()->info, info)
            .param(table()->type, memo->type()->name())
            .param(table()->data, memo->data())
            .exec();
}

QString MemoManager::remove(MemoItem* item) const
{
    return ActionQuery(table()->sqlDelete)
            .param(table()->id, item->id())
            .exec();
    // TODO remove memo specific data
}

QString MemoManager::countAll(int *count) const
{
    SelectQuery query(table()->sqlCountAll());
    if (query.isFailed()) return query.error();

    query.next();
    *count = query.record().value(0).toInt();
    return QString();
}

//------------------------------------------------------------------------------

SettingsTableDef* SettingsManager::table() const { static SettingsTableDef t; return &t; }

void SettingsManager::writeValue(const QString& id, const QVariant& value) const
{
    SelectQuery query(table()->sqlCheckId(id));
    if (query.isFailed())
    {
        qWarning() << "Unable to write setting" << id << query.error();
        return;
    }
    QString sql = query.next() ? table()->sqlUpdate : table()->sqlInsert;
    QString res = ActionQuery(sql)
            .param(table()->id, id)
            .param(table()->value, value)
            .exec();
    if (!res.isEmpty())
        qWarning() << "Error while write setting" << id << res;
}

QVariant SettingsManager::readValue(const QString& id, const QVariant& defValue, bool *hasValue) const
{
    SelectQuery query(table()->sqlSelectById(id));
    if (query.isFailed())
    {
        qWarning() << "Unable to read setting" << id << query.error();
        return defValue;
    }
    if (!query.next())
    {
        if (hasValue)
            *hasValue = false;
        return defValue;
    }
    if (hasValue)
        *hasValue = true;
    return query.record().field(table()->value).value();
}

void SettingsManager::writeBool(const QString& id, bool value) const
{
    bool hasValue;
    bool oldValue = readValue(id, QVariant(), &hasValue).toBool();
    if (!hasValue || oldValue != value)
        writeValue(id, value);
}

bool SettingsManager::readBool(const QString& id, bool defValue) const
{
    return readValue(id, defValue).toBool();
}

void SettingsManager::writeInt(const QString& id, int value) const
{
    bool hasValue;
    int oldValue = readValue(id, QVariant(), &hasValue).toInt();
    if (!hasValue || oldValue != value)
        writeValue(id, value);
}

int SettingsManager::readInt(const QString& id, int defValue) const
{
    return readValue(id, defValue).toInt();
}

void SettingsManager::writeIntArray(const QString& id, const QVector<int>& values, TrackChangesFlag trackChangesFlag) const
{
    if (trackChangesFlag == IgnoreValuesOrder)
    {
        bool doSave = false;
        auto oldValues = readIntArray(id);
        for (int value: oldValues)
            if (!values.contains(value))
            {
                doSave = true;
                break;
            }
        if (!doSave)
            for (int value: values)
                if (!oldValues.contains(value))
                {
                    doSave = true;
                    break;
                }
        if (!doSave) return;
    }

    QString valueStr;
    QStringList strs;
    for (int value: values)
        strs << QString::number(value);
    valueStr = strs.join(';');

    if (trackChangesFlag == RespectValuesOrder)
    {
        auto oldValueStr = readValue(id).toString();
        if (oldValueStr == valueStr) return;
    }

    writeValue(id, valueStr);
}

QVector<int> SettingsManager::readIntArray(const QString& id) const
{
    QVector<int> result;
    QString valuesStr = readValue(id).toString();
    if (!valuesStr.isEmpty())
    {
        bool ok;
        int value;
        for (const QString& valueStr: valuesStr.split(';'))
        {
            value = valueStr.toInt(&ok);
            if (ok) result << value;
        }
    }
    return result;
}

//------------------------------------------------------------------------------

namespace CatalogStore {

QSqlDatabase __db;

QString createTable(TableDef *table);

void closeDatabase()
{
    if (!__db.isOpen()) return;
    QString connection = __db.connectionName();
    __db.close();
    __db = QSqlDatabase();
    QSqlDatabase::removeDatabase(connection);
}

QString newDatabase(const QString fileName)
{
    closeDatabase();

    if (QFile::exists(fileName))
        if (!QFile::remove(fileName))
            return qApp->tr("Unable to overwrite existing file. Probably file is locked.");

    QString res = openDatabase(fileName);
    if (!res.isEmpty())
        return res;

    return QString();
}

QString openDatabase(const QString fileName)
{
    closeDatabase();

    __db = QSqlDatabase::addDatabase("QSQLITE");
    __db.setDatabaseName(fileName);
    if (!__db.open())
        return qApp->tr("Unable to establish a database connection.\n\n%1")
                .arg(SqlHelper::errorText(__db.lastError()));

    QSqlQuery query;
    if (!query.exec("PRAGMA foreign_keys = ON;"))
        return QString("Failed to enable foreign keys.\n\n%1").arg(SqlHelper::errorText(query));

    QString res = beginTran("Setup database structure");
    if (!res.isEmpty()) return res;

    res = createTable(folderManager()->table());
    if (!res.isEmpty()) return res;

    res = createTable(memoManager()->table());
    if (!res.isEmpty()) return res;

    res = createTable(settingsManager()->table());
    if (!res.isEmpty()) return res;

    commitTran();

    return QString();
}

QString createTable(TableDef *table)
{
    auto res = Ori::Sql::ActionQuery(table->sqlCreate()).exec();
    if (!res.isEmpty())
    {
        rollbackTran();
        return QString("Unable to create table '%1'.\n\n%2").arg(table->tableName()).arg(res);
    }
    return QString();
}

QString beginTran(const QString& operation)
{
    if (!__db.transaction())
        return QString("%1: unable to begin transaction.\n\n%1")
                .arg(operation).arg(SqlHelper::errorText(__db.lastError()));
    return QString();
}

void commitTran() { __db.commit(); }
void rollbackTran() { __db.rollback(); }


FolderManager *folderManager() { static FolderManager m; return &m; }
MemoManager* memoManager() { static MemoManager m; return &m; }
SettingsManager* settingsManager() { static SettingsManager m; return &m; }

} // namespace CatalogStore
