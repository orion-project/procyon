#ifndef ENOT_H
#define ENOT_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QDateTime>

#include "core/OriResult.h"

class Enot;
class Folder;
class Memo;
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

class Entry
{
public:
    virtual ~Entry();

    int id() const { return _id; }
    QString title() const { return _title; }
    Entry* parent() const { return _parent; }
    QString path() const;

    bool isRoot() const { return !_parent; }
    bool isFolder() const;
    bool isMemo() const;
    Folder* asFolder();
    Memo* asMemo();

    Folder* parentFolder() { return _parent ? _parent->asFolder() : nullptr; }

private:
    int _id;
    QString _title;
    Entry* _parent = nullptr;

    friend class Enot;
    friend class FolderStore;
    friend class MemoStore;
};

//------------------------------------------------------------------------------

class Folder : public Entry
{
public:
    ~Folder();

    const QList<Folder*>& folders() const { return _folders; }
    const QList<Memo*>& memos() const { return _memos; }

    int childCount() const { return _folders.size() + _memos.size(); }

private:
    QList<Folder*> _folders;
    QList<Memo*> _memos;

    friend class Enot;
    friend class FolderStore;
};

//------------------------------------------------------------------------------

class Memo : public Entry
{
public:
    ~Memo();

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
typedef Ori::Result<Memo*> MemoResult;
typedef Ori::Result<Folder*> FolderResult;
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

    Folder* root() { return &_root; }

    Memo* findMemoById(int id) const;
    Folder* findFolderById(int id) const;

    QString uid() const;
    QString getOrMakeUid();

    IntResult countMemos() const;

    FolderResult createFolder(Folder* parent, const QString& title);
    bool renameFolder(Folder* folder, const QString& title);
    bool removeFolder(Folder* folder);

    MemoResult createMemo(Folder* folder, MemoType *memoType);
    bool updateMemo(Memo* memo, MemoUpdateParam update);
    bool removeMemo(Memo* memo);
    QString loadMemo(Memo* memo);

signals:
    void itemCreating(Entry*, int);
    void itemCreated(Entry*);
    void itemUpdated(Entry*);
    void itemRemoving(Entry*);
    void itemRemoved(Entry*);
    void errorOccurred(const QString& error);

private:
    QString _fileName;
    QString _station;
    Folder _root;
    QMap<int, Memo*> _allMemos;
    QMap<int, Folder*> _allFolders;

    static QString prepareStore(const QString fileName);

    void fillFolderIdsFlat(Folder* root, QVector<int>& ids);
    void fillMemoIdsFlat(Folder* root, QVector<int>& ids);
};

#endif // ENOT_H

