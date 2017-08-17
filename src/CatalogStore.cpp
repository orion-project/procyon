#include "Catalog.h"
#include "CatalogStore.h"
#include "Glass.h"
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

class GlassTableDef : public TableDef
{
public:
    GlassTableDef() : TableDef("Glass") {}

    const QString id = "Id";
    const QString parent = "Parent";
    const QString title = "Title";
    const QString info = "Info";
    const QString comment = "Comment";
    const QString formula = "Formula";
    const QString lambdaMin = "LambdaMin";
    const QString lambdaMax = "LambdaMax";
    const QString coeffs = "Coeffs";

    QString sqlCreate() const override {
        return "CREATE TABLE IF NOT EXISTS Glass ("
               "Id INTEGER PRIMARY KEY, "
               "Parent REFERENCES Folder(Id) ON DELETE CASCADE, "
               "Title, Info, Comment, Formula, LambdaMin, LambdaMax, Coeffs)";
    }

    const QString sqlInsert =
        "INSERT INTO Glass (Id, Parent, Title, Info, Comment, Formula, LambdaMin, LambdaMax, Coeffs) "
        "VALUES (:Id, :Parent, :Title, :Info, :Comment, :Formula, :LambdaMin, :LambdaMax, :Coeffs)";

    const QString sqlUpdate =
        "UPDATE Glass SET Title = :Title, Info = :Info, Comment = :Comment, "
            "Formula = :Formula, LambdaMin = :LambdaMin, LambdaMax = :LambdaMax, Coeffs = :Coeffs "
        "WHERE Id = :Id";

    const QString sqlDelete = "DELETE FROM Glass WHERE Id = :Id";
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

QString serializeCoeffs(const QMap<QString, double>& coeffs)
{
    QStringList strs;
    for (const QString& name : coeffs.keys())
        strs.append(QString("%1=%2").arg(name).arg(coeffs[name], 0, 'g', 16));
    return strs.join(';');
}

QMap<QString, double> deserialzeCoeffs(const QString& str)
{
    QMap<QString, double> coeffs;
    for (const QString& part : str.split(';', QString::SkipEmptyParts))
    {
        int index = part.indexOf('=');
        if (index < 1) continue;
        bool ok;
        double value = QStringRef(&part, index+1, part.length()-index-1).toDouble(&ok);
        if (!ok) continue;
        coeffs[part.left(index)] = value;
    }
    return coeffs;
}


GlassTableDef* GlassManager::table() const { static GlassTableDef t; return &t; }

QString GlassManager::create(GlassItem* item) const
{
    SelectQuery queryId(table()->sqlSelectMaxId());
    if (queryId.isFailed() || !queryId.next())
        return qApp->tr("Unable to generate id for new material.\n\n%1").arg(queryId.error());

    if (!item->glass() || !item->formula())
        return "Bad method usage: poorly defined material: no glass or formula is defined";

    item->glass()->_id = queryId.record().value(0).toInt() + 1;

    auto res = ActionQuery(table()->sqlInsert)
            .param(table()->parent, item->parent() ? item->parent()->asFolder()->id() : 0)
            .param(table()->id, item->glass()->id())
            .param(table()->title, item->glass()->title())
            .param(table()->info, item->info())
            .param(table()->comment, item->glass()->comment())
            .param(table()->lambdaMin, item->glass()->lambdaMin())
            .param(table()->lambdaMax, item->glass()->lambdaMax())
            .param(table()->formula, item->glass()->formula()->name())
            .param(table()->coeffs, serializeCoeffs(item->glass()->coeffValues()))
            .exec();
    if (!res.isEmpty())
        return qApp->tr("Failed to create new material.\n\n%1").arg(res);

    return QString();
}

GlassesResult GlassManager::selectAll() const
{
    GlassesResult result;

    SelectQuery query(table()->sqlSelectAll());
    if (query.isFailed())
    {
        result.error = qApp->tr("Unable to load materials.\n\n%1").arg(query.error());
        return result;
    }

    while (query.next())
    {
        auto r = query.record();

        int id = r.value(table()->id).toInt();
        QString title = r.value(table()->title).toString();
        QString formulaName = r.value(table()->formula).toString();
        if (!dispersionFormulas().contains(formulaName))
        {
            result.warnings.append(qApp->tr("Skip material #%1 '%2': "
                                            "unknown dispersion formula '%3'.")
                                   .arg(id).arg(title).arg(formulaName));
            continue;
        }

        GlassItem *item = new GlassItem;
        item->_id = id;
        item->_title = title;
        item->_info = r.value(table()->info).toString();
        item->_formula = dispersionFormulas()[formulaName];

        int parentId = r.value(table()->parent).toInt();
        if (!result.items.contains(parentId))
            result.items.insert(parentId, QList<GlassItem*>());
        ((QList<GlassItem*>&)result.items[parentId]).append(item);
    }

    return result;
}

QString GlassManager::load(Glass* glass) const
{
    SelectQuery query(table()->sqlSelectById(glass->id()));
    if (query.isFailed())
        return qApp->tr("Unable to load material #%1.\n\n%2").arg(glass->id()).arg(query.error());

    if (!query.next())
        return qApp->tr("Material #%1 does not exist.").arg(glass->id());

    QSqlRecord r = query.record();
    glass->_title = r.value(table()->title).toString();
    glass->_comment = r.value(table()->comment).toString();
    glass->_lambdaMin = r.value(table()->lambdaMin).toDouble();
    glass->_lambdaMax = r.value(table()->lambdaMax).toDouble();
    glass->_coeffValues = deserialzeCoeffs(r.value(table()->coeffs).toString());
    return QString();
}

QString GlassManager::update(Glass* glass, const QString &info) const
{
    return ActionQuery(table()->sqlUpdate)
            .param(table()->id, glass->id())
            .param(table()->title, glass->title())
            .param(table()->info, info)
            .param(table()->comment, glass->comment())
            .param(table()->lambdaMin, glass->lambdaMin())
            .param(table()->lambdaMax, glass->lambdaMax())
            .param(table()->formula, glass->formula()->name())
            .param(table()->coeffs, serializeCoeffs(glass->coeffValues()))
            .exec();
}

QString GlassManager::remove(GlassItem* item) const
{
    return ActionQuery(table()->sqlDelete)
            .param(table()->id, item->glass()->id())
            .exec();
    // TODO remove formula specific values
}

QString GlassManager::countAll(int *count) const
{
    SelectQuery query(table()->sqlCountAll());
    if (query.isFailed()) return query.error();

    query.next();
    *count = query.record().value(0).toInt();
    return QString();
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

    res = createTable(glassManager()->table());
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
GlassManager* glassManager() { static GlassManager m; return &m; }

} // namespace CatalogStore
