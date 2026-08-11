#ifndef ENOT_H
#define ENOT_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QDateTime>

#include "core/OriResult.h"

class Enot;
class FolderItem;
class MemoItem;
class MemoType;

//------------------------------------------------------------------------------

struct MemoUpdateParam
{
    QString title;
    QString data;
    QDateTime moment;
    QString station;
};

//------------------------------------------------------------------------------

class DbItem
{
public:
    virtual ~DbItem();

    int id() const { return _id; }
    QString title() const { return _title; }
    DbItem* parent() const { return _parent; }
    QString path() const;

    bool isRoot() const { return !_parent; }
    bool isFolder() const;
    bool isMemo() const;
    FolderItem* asFolder();
    MemoItem* asMemo();

    FolderItem* parentFolder() { return _parent ? _parent->asFolder() : nullptr; }

private:
    int _id;
    QString _title;
    DbItem* _parent = nullptr;

    friend class Enot;
    friend class FolderStore;
    friend class MemoStore;
};

//------------------------------------------------------------------------------

class FolderItem : public DbItem
{
public:
    ~FolderItem();

    const QList<FolderItem*>& folders() const { return _folders; }
    const QList<MemoItem*>& memos() const { return _memos; }

    int childCount() const { return _folders.size() + _memos.size(); }

private:
    QList<FolderItem*> _folders;
    QList<MemoItem*> _memos;

    friend class Enot;
    friend class FolderStore;
};

//------------------------------------------------------------------------------

class MemoItem : public DbItem
{
public:
    ~MemoItem();

    MemoType* type() { return _type; }
    QString data() const { return _data; }
    QDateTime created() const { return _created; }
    QDateTime updated() const { return _updated; }
    QString station() const { return _station; }
    bool isLoaded() const { return _isLoaded; }

private:
    MemoType* _type = nullptr;
    QString _data, _station;
    bool _isLoaded = false;
    QDateTime _created, _updated;

    friend class Enot;
    friend class MemoStore;
};

//------------------------------------------------------------------------------

typedef Ori::Result<int> IntResult;
typedef Ori::Result<MemoItem*> MemoResult;
typedef Ori::Result<FolderItem*> FolderResult;
typedef Ori::Result<Enot*> EnotResult;

//------------------------------------------------------------------------------

class Enot : public QObject
{
    Q_OBJECT

public:
    Enot(const QString& fileName);
    ~Enot();

    static QString fileFilter();
    static QString defaultFileExt();
    static EnotResult open(const QString& fileName);
    static EnotResult create(const QString& fileName);

    QString fileName() const { return _fileName; }

    FolderItem* root() { return &_root; }

    MemoItem* findMemoById(int id) const;
    FolderItem* findFolderById(int id) const;

    QString uid() const;
    QString getOrMakeUid();

    IntResult countMemos() const;

    FolderResult createFolder(FolderItem* parent, const QString& title);
    bool renameFolder(FolderItem* item, const QString& title);
    bool removeFolder(FolderItem* folder);

    MemoResult createMemo(FolderItem* folder, MemoType *memoType);
    bool updateMemo(MemoItem* item, MemoUpdateParam update);
    bool removeMemo(MemoItem* item);
    QString loadMemo(MemoItem* item);

signals:
    void itemCreating(DbItem*, int);
    void itemCreated(DbItem*);
    void itemUpdated(DbItem*);
    void itemRemoving(DbItem*);
    void itemRemoved(DbItem*);
    void errorOccurred(const QString& error);

private:
    QString _fileName;
    QString _station;
    FolderItem _root;
    QMap<int, MemoItem*> _allMemos;
    QMap<int, FolderItem*> _allFolders;

    static QString prepareStore(const QString fileName);

    void fillFolderIdsFlat(FolderItem* root, QVector<int>& ids);
    void fillMemoIdsFlat(FolderItem* root, QVector<int>& ids);
};

#endif // ENOT_H

