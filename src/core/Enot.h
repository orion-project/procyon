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
    std::optional<QString> title;
    std::optional<QString> data;
    std::optional<QDateTime> moment;
    std::optional<QString> station;
    std::optional<QHash<QString, QString>> props;

    bool IsEmpty() const
    {
        return !title && !data && !moment && !station && !props;
    }
};

//------------------------------------------------------------------------------

class Entry
{
public:
    virtual ~Entry();

    int id() const { return _id; }
    QString title() const { return _title; }
    Folder* parent() const { return _parent; }
    QString path() const;

    bool isFolder() const;
    bool isMemo() const;
    Folder* asFolder();
    Memo* asMemo();

private:
    int _id;
    QString _title;
    Folder* _parent = nullptr;

    friend class Enot;
    friend class MemoStore;
    friend class FolderStore;
};

//------------------------------------------------------------------------------

class Folder : public Entry
{
public:
    ~Folder();

    const QList<Folder*>& folders() const { return _folders; }
    const QList<Memo*>& memos() const { return _memos; }

    int childCount() const { return _folders.size() + _memos.size(); }

    bool isRoot() const { return !parent(); }
    bool isFolder() const = delete;
    bool isMemo() const = delete;
    Folder* asFolder() = delete;
    Memo* asMemo() = delete;

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
    const QHash<QString, QString>& props();

    bool isFolder() const = delete;
    bool isMemo() const = delete;
    Folder* asFolder() = delete;
    Memo* asMemo() = delete;

private:
    MemoType* _type = nullptr;
    QString _data, _station;
    bool _isLoaded = false;
    QDateTime _created, _updated;
    std::optional<QHash<QString, QString>> _props;

    friend class Enot;
    friend class MemoStore;
};

//------------------------------------------------------------------------------

class MemoSheet
{
public:
    int id() const { return _id; }
    QString data() const { return _data; }
    QDateTime created() const { return _created; }
    QDateTime updated() const { return _updated; }
    QString station() const { return _station; }

private:
    int _id;
    QString _data, _station;
    QDateTime _created, _updated;

    friend class MemoStore;
};

//------------------------------------------------------------------------------

class MemoEvent
{
public:
    int memoId() const { return _memoId; }
    QString what() const { return _what; }
    QString value() const { return _value; }
    QDateTime moment() const { return _moment; }
    QString station() const { return _station; }

private:
    int _memoId;
    QString _what, _value, _station;
    QDateTime _moment;

    friend class Enot;
    friend class MemoStore;
};

//------------------------------------------------------------------------------

class MemoLink
{
public:
    int memoId() const { return _memoId; }
    QDateTime created() const { return _created; }
    QString station() const { return _station; }

private:
    int _memoId;
    QString _station;
    QDateTime _created;

    friend class Enot;
    friend class MemoStore;
};

//------------------------------------------------------------------------------

typedef Ori::Result<int> IntResult;
typedef Ori::Result<Enot*> EnotResult;
typedef Ori::Result<Memo*> MemoResult;
typedef Ori::Result<Folder*> FolderResult;

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
    bool deleteFolder(Folder* folder);

    MemoResult createMemo(Folder* folder, MemoType *memoType,
        const std::optional<MemoUpdateParam>& initialData = {});
    bool updateMemo(Memo* memo, MemoUpdateParam update);
    bool deleteMemo(Memo* memo);
    QString loadMemo(Memo* memo);

    QStringList propNames();
    QStringList propValues(const QString& name);
    void addPossiblePropValue(const QString& name, const QString& value);

    void updateMemoOption(int memoId, const QString &name, const QVariant& value);
    void updateMemoProps(Memo* memo, const QHash<QString, QString>& props, const QDateTime& moment);

    void createMemoLink(int id1, int id2);
    void deleteMemoLink(int id1, int id2);

signals:
    void entryCreating(Entry*, int);
    void entryCreated(Entry*);
    void entryUpdated(Entry*);
    void entryDeleting(Entry*);
    void entryDeleted(Entry*);
    void errorOccurred(const QString& error);
    void memoLinkCreated(int id1, int id2);
    void memoLinkDeleted(int id1, int id2);

private:
    QString _fileName;
    QString _station;
    Folder _root;
    QMap<int, Memo*> _allMemos;
    QMap<int, Folder*> _allFolders;
    std::optional<QStringList> _propNames;
    QHash<QString, QStringList> _propValues;

    static QString prepareStore(const QString fileName);

    void fillFolderIdsFlat(Folder* root, QVector<int>& ids);
    void fillMemoIdsFlat(Folder* root, QVector<int>& ids);
};

//------------------------------------------------------------------------------

class MemoOptions
{
public:
inline static const auto& FONT =
#if defined (Q_OS_WIN)
    QStringLiteral("fontWin")
#elif defined (Q_OS_LINUX)
    QStringLiteral("fontLinux")
#elif defined (Q_OS_MAC)
    QStringLiteral("fontMacos")
#else
    QStringLiteral("font")
#endif
    ;
inline static const auto& WORD_WRAP = QStringLiteral("wordWrap");
inline static const auto& SPELLCHECK = QStringLiteral("spellcheck");
inline static const auto& HIGHLIGHTER = QStringLiteral("highlighter");
};

#endif // ENOT_H

