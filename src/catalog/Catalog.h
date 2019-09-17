#ifndef CATALOG_H
#define CATALOG_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QIcon>
#include <QDateTime>

class Catalog;
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
    const char* title() const override { return QT_TRANSLATE_NOOP("MemoType", "Wiki Text"); }
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

//------------------------------------------------------------------------------

class CatalogItem
{
public:
    virtual ~CatalogItem();

    int id() const { return _id; }
    const QString& title() const { return _title; }
    CatalogItem* parent() const { return _parent; }
    const QString path() const;

    bool isFolder() const;
    bool isMemo() const;
    FolderItem* asFolder();
    MemoItem* asMemo();

private:
    int _id;
    QString _title;
    CatalogItem* _parent = nullptr;

    friend class Catalog;
    friend class FolderManager;
    friend class MemoManager;
};

//------------------------------------------------------------------------------

class FolderItem : public CatalogItem
{
public:
    ~FolderItem();

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

    friend class Catalog;
    friend class MemoManager;
};

//------------------------------------------------------------------------------

typedef OperationResult<int> IntResult;
typedef OperationResult<MemoItem*> MemoResult;
typedef OperationResult<FolderItem*> FolderResult;
typedef OperationResult<Catalog*> CatalorResult;

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
    MemoItem* findMemoById(int id) const;
    FolderItem* findFolderById(int id) const;

    IntResult countMemos() const;

    QString renameFolder(FolderItem* item, const QString& title);
    FolderResult createFolder(FolderItem* parent, const QString& title);
    QString removeFolder(FolderItem* item);
    MemoResult createMemo(FolderItem* parent, MemoItem* item);
    QString updateMemo(MemoItem* item, MemoUpdateParam update);
    QString removeMemo(MemoItem* item);
    QString loadMemo(MemoItem* item);

    void fillSubitemsFlat(FolderItem* root, QVector<CatalogItem*> &subitems);
    void fillMemoIdsFlat(FolderItem* root, QVector<int> &ids);

signals:
    void memoCreated(MemoItem*);
    void memoRemoved(MemoItem*);
    void memoUpdated(MemoItem*);

private:
    QString _fileName;
    QString _station;
    QList<CatalogItem*> _items;
    QMap<int, MemoItem*> _allMemos;
    QMap<int, FolderItem*> _allFolders;
};

#endif // CATALOG_H

