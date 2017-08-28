#ifndef CATALOG_H
#define CATALOG_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QIcon>

class Catalog;
class FolderItem;
class MemoItem;
class Memo;

//------------------------------------------------------------------------------

class MemoType
{
public:
    virtual const char* name() const = 0;
    virtual const QIcon& icon() const = 0;
    virtual Memo* makeMemo() = 0;
};

class PlainTextMemoType : public MemoType
{
public:
    const char* name() const { return QT_TRANSLATE_NOOP("MemoType", "Plain Text"); }
    const QIcon& icon() const { static QIcon icon(":/icon/memo_plain_text"); return icon; }
    Memo* makeMemo();
};

class WikiTextMemoType : public MemoType
{
public:
    const char* name() const { return QT_TRANSLATE_NOOP("MemoType", "Wiki Text"); }
    const QIcon& icon() const { static QIcon icon(":/icon/memo_wiki_text"); return icon; }
    Memo* makeMemo();
};

class RichTextMemoType : public MemoType
{
public:
    const char* name() const { return QT_TRANSLATE_NOOP("MemoType", "Rich Text"); }
    const QIcon& icon() const { static QIcon icon(":/icon/memo_rich_text"); return icon; }
    Memo* makeMemo();
};

inline MemoType* plainTextMemoType() { static PlainTextMemoType t; return &t; }
inline MemoType* wikiTextMemoType() { static WikiTextMemoType t; return &t; }
inline MemoType* richTextMemoType() { static RichTextMemoType t; return &t; }

inline const QMap<QString, MemoType*>& memoTypes()
{
    static QMap<QString, MemoType*> memoTypes {
        { plainTextMemoType()->name(), plainTextMemoType() },
        { wikiTextMemoType()->name(), wikiTextMemoType() },
        { richTextMemoType()->name(), richTextMemoType() }
    };
    return memoTypes;
}

//------------------------------------------------------------------------------

template <typename TResult> class OperationResult
{
public:
    TResult result() const { return _result; }
    const QString& error() const { return _error; }
    bool ok() const { return _error.isEmpty(); }

    static OperationResult fail(const QString& error) { return OperationResult(error); }
    static OperationResult ok(TResult result) { return OperationResult(QString(), result); }

private:
    OperationResult(const QString& error): _error(error) {}
    OperationResult(const QString& error, TResult result): _error(error), _result(result) {}

    QString _error;
    TResult _result;
};

typedef OperationResult<Catalog*> CatalorResult;
typedef OperationResult<int> IntResult;

//------------------------------------------------------------------------------

class CatalogItem
{
public:
    virtual ~CatalogItem() {}

    int id() const { return _id; }
    const QString& title() const { return _title; }
    const QString& info() const { return _info; }
    CatalogItem* parent() const { return _parent; }

    bool isFolder() const;
    bool isMemo() const;
    FolderItem* asFolder();
    MemoItem* asMemo();

private:
    int _id;
    QString _title, _info;
    CatalogItem* _parent = nullptr;

    friend class Catalog;
    friend class FolderManager;
    friend class MemoManager;
};

//------------------------------------------------------------------------------

class FolderItem : public CatalogItem
{
public:
    ~FolderItem() { qDeleteAll(_children); }

    const QList<CatalogItem*>& children() const { return _children; }

private:
    QList<CatalogItem*> _children;

    friend class Catalog;
    friend class FolderManager;
};

//------------------------------------------------------------------------------

class MemoItem : public CatalogItem
{
public:
    ~MemoItem();

    Memo* memo() const { return _memo; }
    MemoType* type() { return _type; }

private:
    Memo* _memo = nullptr;
    MemoType* _type = nullptr;

    friend class Catalog;
    friend class MemoManager;
};

//------------------------------------------------------------------------------

class Catalog : public QObject
{
    Q_OBJECT

public:
    Catalog();
    ~Catalog();

    static QString fileFilter();
    static QString defaultFileExt();
    static CatalorResult open(const QString& fileName);
    static CatalorResult create(const QString& fileName);

    const QString& fileName() const { return _fileName; }
    const QList<CatalogItem*>& items() const { return _items; }

    IntResult countMemos() const;

    QString renameFolder(FolderItem* item, const QString& title);
    QString createFolder(FolderItem* parent, const QString& title);
    QString removeFolder(FolderItem* item);
    QString createMemo(FolderItem* parent, Memo *memo);
    QString updateMemo(MemoItem* item, Memo* memo);
    QString removeMemo(MemoItem* item);
    QString loadMemo(MemoItem* item);

signals:
    void memoCreated(MemoItem*);
    void memoRemoved(MemoItem*);
    void memoUpdated(MemoItem*);

private:
    QString _fileName;
    QList<CatalogItem*> _items;
};

#endif // CATALOG_H

