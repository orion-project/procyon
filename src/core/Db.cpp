#include "Db.h"

#include "FolderStore.h"
#include "MemoStore.h"
#include "SettingsManager.h"
#include "SqlHelper.h"

#include <QDebug>
#include <QFile>
#include <QSqlDatabase>
#include <QUuid>

#define KEY_UID "UID"

//------------------------------------------------------------------------------
//                                DbItem
//------------------------------------------------------------------------------

DbItem::~DbItem() {}
bool DbItem::isFolder() const { return dynamic_cast<const FolderItem*>(this); }
bool DbItem::isMemo() const { return dynamic_cast<const MemoItem*>(this); }
FolderItem* DbItem::asFolder() { return dynamic_cast<FolderItem*>(this); }
MemoItem* DbItem::asMemo() { return dynamic_cast<MemoItem*>(this); }

QString DbItem::path() const
{
    QStringList path;
    auto p = _parent;
    while (p)
    {
        path.insert(0, p->title());
        p = p->parent();
    }
    return path.join('/');
}

//------------------------------------------------------------------------------
//                                  FolderItem
//------------------------------------------------------------------------------

FolderItem::~FolderItem()
{
    qDeleteAll(_folders);
    qDeleteAll(_memos);
}

//------------------------------------------------------------------------------
//                                   MemoItem
//------------------------------------------------------------------------------

MemoItem::~MemoItem()
{
}

//------------------------------------------------------------------------------
//                                   Db
//------------------------------------------------------------------------------

QString Db::fileFilter()
{
    return tr("Procyon Notebooks (*.enot);;All files (*.*)");
}

QString Db::defaultFileExt()
{
    return QStringLiteral("enot");
}

QString Db::prepareDb(const QString fileName)
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

    res = Store::folders()->prepare();
    if (!res.isEmpty()) return res;

    res = Store::memos()->prepare();
    if (!res.isEmpty()) return res;

    res = DB::settingsManager()->prepare();
    if (!res.isEmpty()) return res;

    db.commit();
    return QString();
}

DbResult Db::open(const QString& fileName)
{
    QString res = prepareDb(fileName);
    if (!res.isEmpty())
        return DbResult::fail(res);

    Db* db = new Db(fileName);

    // Load folders
    {
        FoldersResult res = Store::folders()->selectAll();
        if (!res.error.isEmpty())
        {
            delete db;
            return DbResult::fail(res.error);
        }

        for (const auto& item : std::as_const(res.items))
            db->_allFolders.insert(item.folder->id(), item.folder);

        for (const auto& item : std::as_const(res.items))
        {
            auto parent = db->_allFolders.value(item.parentId);
            if (!parent)
            {
                parent = db->root();
                qWarning() << QString("Parent folder #%1 not found for folder #%2, reparented to the root")
                                    .arg(item.parentId).arg(item.folder->id());
            }

            item.folder->_parent = parent;
            parent->_folders.append(item.folder);
        }
    }

    // Load memos
    {
        MemosResult res = Store::memos()->selectAll();
        if (!res.error.isEmpty())
        {
            delete db;
            return DbResult::fail(res.error);
        }

        if (!res.warnings.isEmpty())
            for (const auto &warning: std::as_const(res.warnings))
                qWarning() << warning; // TODO make protocol window

        for (const auto& item : std::as_const(res.items))
        {
            auto folder = db->_allFolders.value(item.folderId);
            if (!folder)
            {
                folder = db->root();
                qWarning() << QString("Folder #%1 not found for memo #%2, reparented to the root")
                                  .arg(item.folderId).arg(item.memo->id());
            }

            item.memo->_parent = folder;
            folder->_memos.append(item.memo);
            db->_allMemos[item.memo->id()] = item.memo;
        }
    }

    return DbResult::ok(db);
}

DbResult Db::create(const QString& fileName)
{
    if (QFile::exists(fileName) && !QFile::remove(fileName))
        return DbResult::fail("Unable to overwrite existing file, probably it is locked.");

    QString res = prepareDb(fileName);
    if (!res.isEmpty())
        return DbResult::fail(res);

    Db* db = new Db(fileName);

    return DbResult::ok(db);
}

Db::Db(const QString& fileName) : QObject(), _fileName(fileName)
{
    _root._id = 0;
    _root._title = QFileInfo(fileName).baseName();
    _allFolders.insert(_root.id(), &_root);
}

Db::~Db()
{
    // Don't clear _allMemos and _allFolders explicitly
    // All items will be freed when the root item is deleted
}

bool Db::renameFolder(FolderItem* item, const QString& title)
{
    QString res = Store::folders()->rename(item->id(), title);
    if (!res.isEmpty())
    {
        emit errorOccurred(res);
        return false;
    }

    item->_title = title;

    emit itemUpdated(item);

    // TODO sort items after renaming
    return true;
}

FolderResult Db::createFolder(FolderItem* parent, const QString& title)
{
    if (!parent)
    {
        QString msg = "Parent folder must be provided";
        emit errorOccurred(msg);
        return FolderResult::fail(msg);
    }

    FolderItem* folder = new FolderItem;
    folder->_title = title;
    folder->_parent = parent;

    auto res = Store::folders()->create(folder);
    if (!res.isEmpty())
    {
        delete folder;
        emit errorOccurred(res);
        return FolderResult::fail(res);
    }

    emit itemCreating(folder, parent->_folders.size());

    parent->_folders.append(folder);
    _allFolders.insert(folder->id(), folder);
    // TODO sort items after inserting

    emit itemCreated(folder);
    return FolderResult::ok(folder);
}

bool Db::removeFolder(FolderItem* folder)
{
    if (!folder->parent())
    {
        QString msg = "Unable to remove root folder";
        emit errorOccurred(msg);
        return false;
    }

    QVector<int> folderIds;
    fillFolderIdsFlat(folder, folderIds);

    QVector<int> memoIds;
    fillMemoIdsFlat(folder, memoIds);

    // It removes all subfolders too
    QString res = Store::folders()->remove(folder);
    if (!res.isEmpty())
    {
        emit errorOccurred(res);
        return false;
    }

    emit itemRemoving(folder);

    for (auto id : std::as_const(memoIds))
    {
        auto memo = _allMemos.value(id);
        emit itemRemoving(memo);

        // Memo in DB was already deleted by FK relation
        _allMemos.remove(id);

        emit itemRemoved(memo);
    }

    folder->parentFolder()->_folders.removeOne(folder);

    for (auto id : std::as_const(folderIds))
        _allFolders.remove(id);

    _allFolders.remove(folder->id());

    delete folder;
    return true;
}

MemoResult Db::createMemo(FolderItem* folder, MemoType* memoType)
{
    auto now = QDateTime::currentDateTime();

    auto item = new MemoItem;
    item->_parent = folder;
    item->_created = now;
    item->_updated = now;
    item->_station = _station;
    item->_type = memoType;

    auto res = Store::memos()->create(item);
    if (!res.isEmpty())
    {
        delete item;
        emit errorOccurred(res);
        return MemoResult::fail(res);
    }

    emit itemCreating(item, folder->_memos.size());

    folder->_memos.append(item);
    _allMemos.insert(item->id(), item);
    // TODO sort items after inserting

    emit itemCreated(item);
    return MemoResult::ok(item);
}

bool Db::updateMemo(MemoItem* item, MemoUpdateParam update)
{
    update.moment = QDateTime::currentDateTime();
    update.station = _station;

    QString res = Store::memos()->update(item, update);
    if (!res.isEmpty())
    {
        emit errorOccurred(res);
        return false;
    }

    item->_title = update.title;
    item->_data = update.data;
    item->_updated = update.moment;
    item->_station = update.station;

    emit itemUpdated(item);

    // TODO sort items after renaming
    return true;
}

QString Db::loadMemo(MemoItem* item)
{
    return Store::memos()->load(item);
}

bool Db::removeMemo(MemoItem* item)
{
    QString res = Store::memos()->remove(item);
    if (!res.isEmpty())
    {
        emit errorOccurred(res);
        return false;
    }

    emit itemRemoving(item);

    item->parentFolder()->_memos.removeOne(item);
    _allMemos.remove(item->id());

    emit itemRemoved(item);

    delete item;
    return true;
}

IntResult Db::countMemos() const
{
    int count;
    QString res = Store::memos()->countAll(&count);
    return res.isEmpty() ? IntResult::ok(count) : IntResult::fail(res);
}

namespace {

template <typename TItem>
TItem* findInContainerById(const QMap<int, TItem*>& container, int id)
{
    if (id <= 0)
    {
        qCritical() << "Invalid folder or memo id" << id;
        return nullptr;
    }
    if (!container.contains(id))
    {
        qCritical() << "Inconsistent state! Db does not contain folder or memo" << id;
        return nullptr;
    }
    return container[id];
}

} // namespace

MemoItem* Db::findMemoById(int id) const
{
    return findInContainerById(_allMemos, id);
}

FolderItem* Db::findFolderById(int id) const
{
    return findInContainerById(_allFolders, id);
}

void Db::fillFolderIdsFlat(FolderItem* root, QVector<int>& ids)
{
    for (auto folder : root->folders())
    {
        ids.append(folder->id());
        fillFolderIdsFlat(folder, ids);
    }
}

void Db::fillMemoIdsFlat(FolderItem* root, QVector<int> &ids)
{
    for (auto memo : root->memos())
        ids.append(memo->id());

    for (auto folder : root->folders())
        fillMemoIdsFlat(folder, ids);
}

QString Db::uid() const
{
    return DB::settingsManager()->readString(KEY_UID);
}

QString Db::getOrMakeUid()
{
    QString uid = DB::settingsManager()->readString(KEY_UID);
    if (uid.isEmpty())
    {
        uid = QUuid::createUuid().toString();
        DB::settingsManager()->writeString(KEY_UID, uid);
    }
    return uid;
}
