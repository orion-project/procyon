#include "Db.h"

#include "FolderManager.h"
#include "MemoManager.h"
#include "SettingsManager.h"
#include "SqlHelper.h"

#include <QDebug>
#include <QFile>
#include <QSqlDatabase>
#include <QUuid>

#define KEY_UID "UID"

//------------------------------------------------------------------------------
//                                MemoType
//------------------------------------------------------------------------------

MemoType::~MemoType()
{
}

class PlainTextMemoType : public MemoType
{
public:
    const QString name() const override { return QStringLiteral("plain_text"); }
    const char* title() const override { return QT_TRANSLATE_NOOP("MemoType", "Plain Text"); }
    const QIcon& icon() const override { static QIcon icon(iconPath()); return icon; }
    const QString iconPath() const override { return QStringLiteral(":/icon/memo_plain_text"); }
};

class MarkdownMemoType : public MemoType
{
public:
    const QString name() const override { return QStringLiteral("markdown"); }
    const char* title() const override { return QT_TRANSLATE_NOOP("MemoType", "Markdown"); }
    const QIcon& icon() const override { static QIcon icon(iconPath()); return icon; }
    const QString iconPath() const override { return QStringLiteral(":/icon/memo_markdown"); }
};

class RichTextMemoType : public MemoType
{
public:
    const QString name() const override { return QStringLiteral("rich_text"); }
    const char* title() const override { return QT_TRANSLATE_NOOP("MemoType", "Rich Text"); }
    const QIcon& icon() const override { static QIcon icon(iconPath()); return icon; }
    const QString iconPath() const override { return QStringLiteral(":/icon/memo_rich_text"); }
};

MemoType* plainTextMemoType() { static PlainTextMemoType t; return &t; }
MemoType* markdownMemoType() { static MarkdownMemoType t; return &t; }
MemoType* richTextMemoType() { static RichTextMemoType t; return &t; }

const QMap<QString, MemoType*>& memoTypes()
{
    static QMap<QString, MemoType*> memoTypes {
        { plainTextMemoType()->name(), plainTextMemoType() },
        { markdownMemoType()->name(), markdownMemoType() },
        { richTextMemoType()->name(), richTextMemoType() }
    };
    return memoTypes;
}

MemoType* getMemoType(const QString& type)
{
    auto allTypes = memoTypes();
    if (!allTypes.contains(type))
        return plainTextMemoType();
    return allTypes[type];
}

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
    qDeleteAll(_children);
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

    res = DB::folderManager()->prepare();
    if (!res.isEmpty()) return res;

    res = DB::memoManager()->prepare();
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
        FoldersResult res = DB::folderManager()->selectAll();
        if (!res.error.isEmpty())
        {
            delete db;
            return DbResult::fail(res.error);
        }

        for (const auto& item : std::as_const(res.items))
        {
            auto parent = db->_allFolders.value(item.parentId);
            if (!parent)
            {
                qWarning() << QString("Parent folder #%1 not found for folder #%2")
                                    .arg(item.parentId).arg(item.folder->id());
                delete item.folder;
                continue;
            }

            item.folder->_parent = parent;
            parent->_children.append(item.folder);
            db->_allFolders.insert(item.folder->id(), item.folder);
        }
    }

    // Load memos
    {
        MemosResult res = DB::memoManager()->selectAll();
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
                qWarning() << QString("Folder #%1 not found for memo #%2")
                                  .arg(item.folderId).arg(item.memo->id());
                delete item.memo;
                continue;
            }

            item.memo->_parent = folder;
            folder->_children.append(item.memo);
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

QString Db::renameFolder(FolderItem* item, const QString& title)
{
    QString res = DB::folderManager()->rename(item->id(), title);
    if (!res.isEmpty()) return res;

    item->_title = title;

    // TODO sort items after renaming
    return QString();
}

FolderResult Db::createFolder(FolderItem* parent, const QString& title)
{
    if (!parent)
        return FolderResult::fail("Parent folder must be provided");

    FolderItem* folder = new FolderItem;
    folder->_title = title;
    folder->_parent = parent;

    auto res = DB::folderManager()->create(folder);
    if (!res.isEmpty())
    {
        delete folder;
        return FolderResult::fail(res);
    }

    parent->_children.append(folder);
    _allFolders.insert(folder->id(), folder);
    // TODO sort items after inserting

    return FolderResult::ok(folder);
}

QString Db::removeFolder(FolderItem* folder)
{
    if (!folder->parent())
        return "Unable to remove root folder";

    QVector<DbItem*> subitems;
    fillSubitemsFlat(folder, subitems);

    // It removes all subfolders too
    QString res = DB::folderManager()->remove(folder);
    if (!res.isEmpty()) return res;

    folder->parentFolder()->_children.removeOne(folder);

    for (auto subitem : std::as_const(subitems))
    {
        if (subitem->isFolder())
            _allFolders.remove(subitem->id());
        else
        {
            // Memo in DB was already deleted by FK relation
            emit memoRemoved(dynamic_cast<MemoItem*>(subitem));
            _allMemos.remove(subitem->id());
        }
    }

    _allFolders.remove(folder->id());
    delete folder;

    return QString();
}

MemoResult Db::createMemo(FolderItem* folder, MemoItem* item, MemoType* memoType)
{
    auto now = QDateTime::currentDateTime();

    item->_parent = folder;
    item->_created = now;
    item->_updated = now;
    item->_station = _station;
    item->_type = memoType;

    auto res = DB::memoManager()->create(item);
    if (!res.isEmpty())
    {
        delete item;
        return MemoResult::fail(res);
    }

    folder->_children.append(item);
    _allMemos.insert(item->id(), item);
    // TODO sort items after inserting

    emit memoCreated(item);

    return MemoResult::ok(item);
}

QString Db::updateMemo(MemoItem* item, MemoUpdateParam update)
{
    update.moment = QDateTime::currentDateTime();
    update.station = _station;

    QString res = DB::memoManager()->update(item, update);
    if (!res.isEmpty()) return res;

    item->_title = update.title;
    item->_data = update.data;
    item->_updated = update.moment;
    item->_station = update.station;

    emit memoUpdated(item);

    // TODO sort items after renaming
    return QString();
}

QString Db::loadMemo(MemoItem* item)
{
    return DB::memoManager()->load(item);
}

QString Db::removeMemo(MemoItem* item)
{
    QString res = DB::memoManager()->remove(item);
    if (!res.isEmpty()) return res;

    item->parentFolder()->_children.removeOne(item);
    _allMemos.remove(item->id());

    emit memoRemoved(item);

    delete item;
    return QString();
}

IntResult Db::countMemos() const
{
    int count;
    QString res = DB::memoManager()->countAll(&count);
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

void Db::fillSubitemsFlat(FolderItem* root, QVector<DbItem*>& subitems)
{
    for (DbItem* item : root->children())
    {
        subitems.append(item);

        if (item->isFolder())
            fillSubitemsFlat(item->asFolder(), subitems);
    }
}

void Db::fillMemoIdsFlat(FolderItem* root, QVector<int> &ids)
{
    for (DbItem* item : root->children())
    {
        if (item->isFolder())
            fillMemoIdsFlat(item->asFolder(), ids);
        else ids.append(item->id());
    }
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
