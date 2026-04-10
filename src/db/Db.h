#ifndef DB_H
#define DB_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QIcon>
#include <QDateTime>

#include "core/OriResult.h"

class Db;
class FolderItem;
class MemoItem;

//------------------------------------------------------------------------------

class MemoType
{
public:
    virtual ~MemoType();
    virtual const QString name() const = 0;
    virtual const char* title() const = 0;
    virtual const QIcon& icon() const = 0;
    virtual const QString iconPath() const = 0;
};

MemoType* plainTextMemoType();
MemoType* markdownMemoType();
MemoType* richTextMemoType();
const QMap<QString, MemoType*>& memoTypes();
MemoType* getMemoType(const QString& type);

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
    const QString& title() const { return _title; }
    DbItem* parent() const { return _parent; }
    const QString path() const;

    bool isFolder() const;
    bool isMemo() const;
    FolderItem* asFolder();
    MemoItem* asMemo();

private:
    int _id;
    QString _title;
    DbItem* _parent = nullptr;

    friend class Db;
    friend class FolderManager;
    friend class MemoManager;
};

//------------------------------------------------------------------------------

class FolderItem : public DbItem
{
public:
    ~FolderItem();

    const QList<DbItem*>& children() const { return _children; }

private:
    QList<DbItem*> _children;

    friend class Db;
    friend class FolderManager;
};

//------------------------------------------------------------------------------

class MemoItem : public DbItem
{
public:
    ~MemoItem();

    MemoType* type() { return _type; }
    const QString& data() const { return _data; }
    const QDateTime& created() const { return _created; }
    const QDateTime& updated() const { return _updated; }
    const QString& station() const { return _station; }
    bool isLoaded() const { return _isLoaded; }

private:
    MemoType* _type = nullptr;
    QString _data, _station;
    bool _isLoaded = false;
    QDateTime _created, _updated;

    friend class Db;
    friend class MemoManager;
};

//------------------------------------------------------------------------------

typedef Ori::Result<int> IntResult;
typedef Ori::Result<MemoItem*> MemoResult;
typedef Ori::Result<FolderItem*> FolderResult;
typedef Ori::Result<Db*> DbResult;

//------------------------------------------------------------------------------

class Db : public QObject
{
    Q_OBJECT

public:
    Db();
    ~Db();

    static QString fileFilter();
    static QString defaultFileExt();
    static DbResult open(const QString& fileName);
    static DbResult create(const QString& fileName);

    const QString& fileName() const { return _fileName; }
    const QList<DbItem*>& topItems() const { return _topItems; }
    MemoItem* findMemoById(int id) const;
    FolderItem* findFolderById(int id) const;

    QString uid() const;
    QString getOrMakeUid();

    IntResult countMemos() const;

    QString renameFolder(FolderItem* item, const QString& title);
    FolderResult createFolder(FolderItem* parent, const QString& title);
    QString removeFolder(FolderItem* item);
    MemoResult createMemo(FolderItem* parent, MemoItem* item, MemoType *memoType);
    QString updateMemo(MemoItem* item, MemoUpdateParam update);
    QString removeMemo(MemoItem* item);
    QString loadMemo(MemoItem* item);

    void fillSubitemsFlat(FolderItem* root, QVector<DbItem*> &subitems);
    void fillMemoIdsFlat(FolderItem* root, QVector<int> &ids);

signals:
    void memoCreated(MemoItem*);
    void memoRemoved(MemoItem*);
    void memoUpdated(MemoItem*);

private:
    QString _fileName;
    QString _station;
    QList<DbItem*> _topItems;
    QMap<int, MemoItem*> _allMemos;
    QMap<int, FolderItem*> _allFolders;

    static QString prepareDb(const QString fileName);
};

#endif // DB_H

