#include "Enot.h"

#include "FolderStore.h"
#include "MemoStore.h"
#include "SettingsStore.h"
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
bool DbItem::isFolder() const { return dynamic_cast<const Folder*>(this); }
bool DbItem::isMemo() const { return dynamic_cast<const Memo*>(this); }
Folder* DbItem::asFolder() { return dynamic_cast<Folder*>(this); }
Memo* DbItem::asMemo() { return dynamic_cast<Memo*>(this); }

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
//                                  Folder
//------------------------------------------------------------------------------

Folder::~Folder()
{
    qDeleteAll(_folders);
    qDeleteAll(_memos);
}

//------------------------------------------------------------------------------
//                                   Memo
//------------------------------------------------------------------------------

Memo::~Memo()
{
}

//------------------------------------------------------------------------------
//                                   Enot
//------------------------------------------------------------------------------

QString Enot::fileFilter()
{
    return tr("Procyon Notebooks (*.enot);;All files (*.*)");
}

QString Enot::defaultFileExt()
{
    return QStringLiteral("enot");
}

QString Enot::prepareStore(const QString fileName)
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

    res = Store::settings()->prepare();
    if (!res.isEmpty()) return res;

    db.commit();
    return QString();
}

EnotResult Enot::open(const QString& fileName)
{
    QString res = prepareStore(fileName);
    if (!res.isEmpty())
        return EnotResult::fail(res);

    Enot* enot = new Enot(fileName);

    // Load folders
    {
        FoldersResult res = Store::folders()->selectAll();
        if (!res.error.isEmpty())
        {
            delete enot;
            return EnotResult::fail(res.error);
        }

        for (const auto& item : std::as_const(res.items))
            enot->_allFolders.insert(item.folder->id(), item.folder);

        for (const auto& item : std::as_const(res.items))
        {
            auto parent = enot->_allFolders.value(item.parentId);
            if (!parent)
            {
                parent = enot->root();
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
            delete enot;
            return EnotResult::fail(res.error);
        }

        if (!res.warnings.isEmpty())
            for (const auto &warning: std::as_const(res.warnings))
                qWarning() << warning; // TODO make protocol window

        for (const auto& item : std::as_const(res.items))
        {
            auto folder = enot->_allFolders.value(item.folderId);
            if (!folder)
            {
                folder = enot->root();
                qWarning() << QString("Folder #%1 not found for memo #%2, reparented to the root")
                                  .arg(item.folderId).arg(item.memo->id());
            }

            item.memo->_parent = folder;
            folder->_memos.append(item.memo);
            enot->_allMemos[item.memo->id()] = item.memo;
        }
    }

    return EnotResult::ok(enot);
}

EnotResult Enot::create(const QString& fileName)
{
    if (QFile::exists(fileName) && !QFile::remove(fileName))
        return EnotResult::fail("Unable to overwrite existing file, probably it is locked.");

    QString res = prepareStore(fileName);
    if (!res.isEmpty())
        return EnotResult::fail(res);

    Enot* enot = new Enot(fileName);

    return EnotResult::ok(enot);
}

Enot::Enot(const QString& fileName) : QObject(), _fileName(fileName)
{
    _root._id = 0;
    _root._title = QFileInfo(fileName).baseName();
    _allFolders.insert(_root.id(), &_root);
}

Enot::~Enot()
{
    // Don't clear _allMemos and _allFolders explicitly
    // All items will be freed when the root item is deleted
}

bool Enot::renameFolder(Folder* item, const QString& title)
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

FolderResult Enot::createFolder(Folder* parent, const QString& title)
{
    if (!parent)
    {
        QString msg = "Parent folder must be provided";
        emit errorOccurred(msg);
        return FolderResult::fail(msg);
    }

    Folder* folder = new Folder;
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

bool Enot::removeFolder(Folder* folder)
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

MemoResult Enot::createMemo(Folder* folder, MemoType* memoType)
{
    auto now = QDateTime::currentDateTime();

    auto item = new Memo;
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

bool Enot::updateMemo(Memo* item, MemoUpdateParam update)
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

QString Enot::loadMemo(Memo* item)
{
    return Store::memos()->load(item);
}

bool Enot::removeMemo(Memo* item)
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

IntResult Enot::countMemos() const
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

Memo* Enot::findMemoById(int id) const
{
    return findInContainerById(_allMemos, id);
}

Folder* Enot::findFolderById(int id) const
{
    return findInContainerById(_allFolders, id);
}

void Enot::fillFolderIdsFlat(Folder* root, QVector<int>& ids)
{
    for (auto folder : root->folders())
    {
        ids.append(folder->id());
        fillFolderIdsFlat(folder, ids);
    }
}

void Enot::fillMemoIdsFlat(Folder* root, QVector<int> &ids)
{
    for (auto memo : root->memos())
        ids.append(memo->id());

    for (auto folder : root->folders())
        fillMemoIdsFlat(folder, ids);
}

QString Enot::uid() const
{
    return Store::settings()->readString(KEY_UID);
}

QString Enot::getOrMakeUid()
{
    QString uid = Store::settings()->readString(KEY_UID);
    if (uid.isEmpty())
    {
        uid = QUuid::createUuid().toString();
        Store::settings()->writeString(KEY_UID, uid);
    }
    return uid;
}
