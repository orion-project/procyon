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
//                                Entry
//------------------------------------------------------------------------------

Entry::~Entry() {}
bool Entry::isFolder() const { return dynamic_cast<const Folder*>(this); }
bool Entry::isMemo() const { return dynamic_cast<const Memo*>(this); }
Folder* Entry::asFolder() { return dynamic_cast<Folder*>(this); }
Memo* Entry::asMemo() { return dynamic_cast<Memo*>(this); }

QString Entry::path() const
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

const QHash<QString, QString>& Memo::props()
{
    if (!_props)
        _props = Store::memos()->loadProps(id());
    return _props.value();
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
    // All entries will be freed when the root folder is deleted
}

bool Enot::renameFolder(Folder* folder, const QString& title)
{
    QString res = Store::folders()->rename(folder->id(), title);
    if (!res.isEmpty())
    {
        emit errorOccurred(res);
        return false;
    }

    folder->_title = title;

    emit entryUpdated(folder);

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

    emit entryCreating(folder, parent->_folders.size());

    parent->_folders.append(folder);
    _allFolders.insert(folder->id(), folder);
    // TODO sort items after inserting

    emit entryCreated(folder);
    return FolderResult::ok(folder);
}

bool Enot::deleteFolder(Folder* folder)
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

    emit entryDeleting(folder);

    for (auto id : std::as_const(memoIds))
    {
        auto memo = _allMemos.value(id);
        emit entryDeleting(memo);

        // Memo in DB was already deleted by FK relation
        _allMemos.remove(id);

        emit entryDeleted(memo);
    }

    folder->parent()->_folders.removeOne(folder);

    for (auto id : std::as_const(folderIds))
        _allFolders.remove(id);

    _allFolders.remove(folder->id());

    delete folder;
    return true;
}

MemoResult Enot::createMemo(Folder* folder, MemoType* memoType)
{
    auto now = QDateTime::currentDateTime();

    auto memo = new Memo;
    memo->_parent = folder;
    memo->_created = now;
    memo->_updated = now;
    memo->_station = _station;
    memo->_type = memoType;

    auto res = Store::memos()->create(memo);
    if (!res.isEmpty())
    {
        delete memo;
        emit errorOccurred(res);
        return MemoResult::fail(res);
    }

    emit entryCreating(memo, folder->_memos.size());

    folder->_memos.append(memo);
    _allMemos.insert(memo->id(), memo);
    // TODO sort items after inserting

    emit entryCreated(memo);
    return MemoResult::ok(memo);
}

bool Enot::updateMemo(Memo* memo, MemoUpdateParam update)
{
    update.moment = QDateTime::currentDateTime();
    update.station = _station;

    QString res = Store::memos()->update(memo, update);
    if (!res.isEmpty())
    {
        emit errorOccurred(res);
        return false;
    }

    memo->_title = update.title;
    memo->_data = update.data;
    memo->_updated = update.moment;
    memo->_station = update.station;

    if (update.props)
    {
        QStringList errors;

        auto names = memo->props().keys();
        for (const auto& name : std::as_const(names))
        {
            if (!update.props->contains(name))
            {
                auto err = Store::memos()->deleteProp(memo->id(), name);
                if (!err.isEmpty())
                    errors << err;
                else
                    memo->_props->remove(name);
            }
        }

        for (auto it = update.props->cbegin(); it != update.props->cend(); it++)
        {
            auto name = it.key();
            auto value = it.value();
            if (value != memo->_props->value(name))
            {
                auto err = Store::memos()->updateProp(memo->id(), name, value);
                if (!err.isEmpty())
                    errors << err;
                else
                    memo->_props->insert(name, value);
            }
        }

        if (!errors.isEmpty())
        {
            emit errorOccurred(errors.join('\n'));
            // Don't return false here,
            // since the memo itself is saved successfully
        }
    }

    emit entryUpdated(memo);

    // TODO sort memos after renaming
    return true;
}

QString Enot::loadMemo(Memo* memo)
{
    return Store::memos()->load(memo);
}

bool Enot::deleteMemo(Memo* memo)
{
    QString res = Store::memos()->remove(memo);
    if (!res.isEmpty())
    {
        emit errorOccurred(res);
        return false;
    }

    emit entryDeleting(memo);

    memo->parent()->_memos.removeOne(memo);
    _allMemos.remove(memo->id());

    emit entryDeleted(memo);

    delete memo;
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

QStringList Enot::propNames()
{
    if (!_propNames)
        _propNames = Store::memos()->loadPropNames();
    return _propNames.value();
}

QStringList Enot::propValues(const QString& name)
{
    if (!_propValues.contains(name))
        _propValues.insert(name, Store::memos()->loadPropValues(name));
    return _propValues.value(name);
}

void Enot::addPossiblePropValue(const QString& name, const QString& value)
{
    if (!_propNames->contains(name))
    {
        _propNames->append(name);
        _propNames->sort();
    }

    auto& values = _propValues[name];
    if (!values.contains(value))
    {
        values.append(value);
        values.sort();
    }
}
