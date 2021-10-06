#include "Catalog.h"
#include "CatalogStore.h"

#include <QDebug>
#include <QUuid>
#include <QFile>

static const QString KEY_UID("UID");

//------------------------------------------------------------------------------
//                                MemoType
//------------------------------------------------------------------------------

MemoType::~MemoType()
{
}

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
//                                CatalogItem
//------------------------------------------------------------------------------

CatalogItem::~CatalogItem() {}
bool CatalogItem::isFolder() const { return dynamic_cast<const FolderItem*>(this); }
bool CatalogItem::isMemo() const { return dynamic_cast<const MemoItem*>(this); }
FolderItem* CatalogItem::asFolder() { return dynamic_cast<FolderItem*>(this); }
MemoItem* CatalogItem::asMemo() { return dynamic_cast<MemoItem*>(this); }

const QString CatalogItem::path() const
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
//                                   Catalog
//------------------------------------------------------------------------------

QString Catalog::fileFilter()
{
    return tr("Procyon Notebooks (*.enot);;All files (*.*)");
}

QString Catalog::defaultFileExt()
{
    return QStringLiteral("enot");
}

CatalorResult Catalog::open(const QString& fileName)
{
    QString res = CatalogStore::openDatabase(fileName);
    if (!res.isEmpty())
        return CatalorResult::fail(res);

    Catalog* catalog = new Catalog;
    catalog->_fileName = fileName;

    FoldersResult folders = CatalogStore::folderManager()->selectAll();
    if (!folders.error.isEmpty())
    {
        delete catalog;
        return CatalorResult::fail(folders.error);
    }

    for (FolderItem* item: folders.items.values())
    {
        catalog->_allFolders[item->id()] = item;

        if (!item->parent())
            catalog->_items.append(item);
    }

    MemosResult memos = CatalogStore::memoManager()->selectAll();
    if (!memos.error.isEmpty())
    {
        delete catalog;
        return CatalorResult::fail(memos.error);
    }

    if (!memos.warnings.isEmpty())
        for (auto warning: memos.warnings)
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
                catalog->_items.append(item);
    }

    catalog->_allMemos = memos.allMemos;

    return CatalorResult::ok(catalog);
}

CatalorResult Catalog::create(const QString& fileName)
{
    if (QFile::exists(fileName) && !QFile::remove(fileName))
        return CatalorResult::fail(QString("Unable to overwrite existing file, probably it is locked."));

    QString res = CatalogStore::openDatabase(fileName);
    if (!res.isEmpty())
        return CatalorResult::fail(res);

    Catalog* catalog = new Catalog;
    catalog->_fileName = fileName;

    return CatalorResult::ok(catalog);
}

Catalog::Catalog() : QObject()
{
}

Catalog::~Catalog()
{
    qDeleteAll(_items);
}

QString Catalog::renameFolder(FolderItem* item, const QString& title)
{
    QString res = CatalogStore::folderManager()->rename(item->id(), title);
    if (!res.isEmpty()) return res;

    item->_title = title;

    // TODO sort items after renaming
    return QString();
}

FolderResult Catalog::createFolder(FolderItem* parent, const QString& title)
{
    FolderItem* folder = new FolderItem;
    folder->_title = title;
    folder->_parent = parent;

    auto res = CatalogStore::folderManager()->create(folder);
    if (!res.isEmpty())
    {
        delete folder;
        return FolderResult::fail(res);
    }

    (parent ? parent->_children : _items).append(folder);
    _allFolders.insert(folder->id(), folder);
    // TODO sort items after inserting

    return FolderResult::ok(folder);
}

QString Catalog::removeFolder(FolderItem* item)
{
    QVector<CatalogItem*> subitems;
    fillSubitemsFlat(item, subitems);

    // It removes all subfolders too
    QString res = CatalogStore::folderManager()->remove(item);
    if (!res.isEmpty()) return res;

    (item->parent() ? item->parent()->asFolder()->_children : _items).removeOne(item);

    for (auto subitem : subitems)
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

MemoResult Catalog::createMemo(FolderItem* parent, MemoItem* item, MemoType* memoType)
{
    auto now = QDateTime::currentDateTime();

    item->_parent = parent;
    item->_created = now;
    item->_updated = now;
    item->_station = _station;
    item->_type = memoType;

    auto res = CatalogStore::memoManager()->create(item);
    if (!res.isEmpty())
    {
        delete item;
        return MemoResult::fail(res);
    }

    (parent ? parent->asFolder()->_children : _items).append(item);
    _allMemos.insert(item->id(), item);
    // TODO sort items after inserting

    emit memoCreated(item);

    return MemoResult::ok(item);
}

QString Catalog::updateMemo(MemoItem* item, MemoUpdateParam update)
{
    update.moment = QDateTime::currentDateTime();
    update.station = _station;

    QString res = CatalogStore::memoManager()->update(item, update);
    if (!res.isEmpty()) return res;

    item->_title = update.title;
    item->_data = update.data;
    item->_updated = update.moment;
    item->_station = update.station;

    emit memoUpdated(item);

    // TODO sort items after renaming
    return QString();
}

QString Catalog::loadMemo(MemoItem* item)
{
    return CatalogStore::memoManager()->load(item);
}

QString Catalog::removeMemo(MemoItem* item)
{
    QString res = CatalogStore::memoManager()->remove(item);
    if (!res.isEmpty()) return res;

    (item->parent() ? item->parent()->asFolder()->_children : _items).removeOne(item);
    _allMemos.remove(item->id());

    emit memoRemoved(item);

    delete item;
    return QString();
}

IntResult Catalog::countMemos() const
{
    int count;
    QString res = CatalogStore::memoManager()->countAll(&count);
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
        qCritical() << "Inconsistent state! Catalog does not contain folder or memo" << id;
        return nullptr;
    }
    return container[id];
}

} // namespace

MemoItem* Catalog::findMemoById(int id) const
{
    return findInContainerById(_allMemos, id);
}

FolderItem* Catalog::findFolderById(int id) const
{
    return findInContainerById(_allFolders, id);
}

void Catalog::fillSubitemsFlat(FolderItem* root, QVector<CatalogItem*>& subitems)
{
    for (CatalogItem* item : root->children())
    {
        subitems.append(item);

        if (item->isFolder())
            fillSubitemsFlat(item->asFolder(), subitems);
    }
}

void Catalog::fillMemoIdsFlat(FolderItem* root, QVector<int> &ids)
{
    for (CatalogItem* item : root->children())
    {
        if (item->isFolder())
            fillMemoIdsFlat(item->asFolder(), ids);
        else ids.append(item->id());
    }
}

QString Catalog::uid() const
{
    return CatalogStore::settingsManager()->readString(KEY_UID);
}

QString Catalog::getOrMakeUid()
{
    QString uid = CatalogStore::settingsManager()->readString(KEY_UID);
    if (uid.isEmpty())
    {
        uid = QUuid::createUuid().toString();
        CatalogStore::settingsManager()->writeString(KEY_UID, uid);
    }
    return uid;
}
