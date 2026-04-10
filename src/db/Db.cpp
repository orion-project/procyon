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

const QString DbItem::path() const
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

    Db* db = new Db;
    db->_fileName = fileName;

    FoldersResult folders = DB::folderManager()->selectAll();
    if (!folders.error.isEmpty())
    {
        delete db;
        return DbResult::fail(folders.error);
    }

    for (FolderItem* item: folders.items.values())
    {
        db->_allFolders[item->id()] = item;

        if (!item->parent())
            db->_topItems.append(item);
    }

    MemosResult memos = DB::memoManager()->selectAll();
    if (!memos.error.isEmpty())
    {
        delete db;
        return DbResult::fail(memos.error);
    }

    if (!memos.warnings.isEmpty())
        for (const auto &warning: std::as_const(memos.warnings))
            qWarning() << warning; // TODO make protocol window

    for (int folderId: memos.items.keys())
    {
        if (folderId > 0)
        {
            if (!folders.items.contains(folderId))
            {
                qWarning() << tr("Some memos are stored in folder #%1 but that "
                                 "is not found in the directory.").arg(folderId);
                qDeleteAll(memos.items[folderId]);
                continue;
            }
            FolderItem *parent = folders.items[folderId];
            for (MemoItem* item: memos.items[folderId])
            {
                item->_parent = parent;
                parent->_children.append(item);
            }
        }
        else
            for (MemoItem* item: memos.items[folderId])
                db->_topItems.append(item);
    }

    db->_allMemos = memos.allMemos;

    return DbResult::ok(db);
}

DbResult Db::create(const QString& fileName)
{
    if (QFile::exists(fileName) && !QFile::remove(fileName))
        return DbResult::fail(QString("Unable to overwrite existing file, probably it is locked."));

    QString res = prepareDb(fileName);
    if (!res.isEmpty())
        return DbResult::fail(res);

    Db* db = new Db;
    db->_fileName = fileName;

    return DbResult::ok(db);
}

Db::Db() : QObject()
{
}

Db::~Db()
{
    qDeleteAll(_topItems);
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
    FolderItem* folder = new FolderItem;
    folder->_title = title;
    folder->_parent = parent;

    auto res = DB::folderManager()->create(folder);
    if (!res.isEmpty())
    {
        delete folder;
        return FolderResult::fail(res);
    }

    (parent ? parent->_children : _topItems).append(folder);
    _allFolders.insert(folder->id(), folder);
    // TODO sort items after inserting

    return FolderResult::ok(folder);
}

QString Db::removeFolder(FolderItem* item)
{
    QVector<DbItem*> subitems;
    fillSubitemsFlat(item, subitems);

    // It removes all subfolders too
    QString res = DB::folderManager()->remove(item);
    if (!res.isEmpty()) return res;

    (item->parent() ? item->parent()->asFolder()->_children : _topItems).removeOne(item);

    for (auto subitem : std::as_const(subitems))
        if (subitem->isFolder())
            _allFolders.remove(subitem->id());
        else
        {
            // Memo in DB was already deleted by FK relation
            emit memoRemoved(dynamic_cast<MemoItem*>(subitem));
            _allMemos.remove(subitem->id());
        }

    _allFolders.remove(item->id());

    delete item;
    return QString();
}

MemoResult Db::createMemo(FolderItem* parent, MemoItem* item, MemoType* memoType)
{
    auto now = QDateTime::currentDateTime();

    item->_parent = parent;
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

    (parent ? parent->asFolder()->_children : _topItems).append(item);
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

    (item->parent() ? item->parent()->asFolder()->_children : _topItems).removeOne(item);
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
