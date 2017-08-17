#ifndef CATALOG_H
#define CATALOG_H

#include <QObject>
#include <QList>
#include <QMap>
#include <QIcon>

class Catalog;
class FolderItem;
class GlassItem;
class Glass;

//------------------------------------------------------------------------------

class DispersionFormula
{
public:
    virtual const char* name() const = 0;
    virtual const QIcon& icon() const = 0;
    virtual QStringList coeffNames() const { return QStringList(); }
    virtual Glass* makeGlass() = 0;
};

class ShottFormula : public DispersionFormula
{
public:
    const char* name() const { return QT_TRANSLATE_NOOP("Formula", "Shott"); }
    const QIcon& icon() const { static QIcon icon(":/icon/glass_green"); return icon; }
    QStringList coeffNames() const { static QStringList c(
        {"c_1", "c_2", "c_3", "c_4", "c_5", "c_6"}); return c; }
    Glass* makeGlass();
};

class SellmeierFormula : public DispersionFormula
{
public:
    const char* name() const { return QT_TRANSLATE_NOOP("Formula", "Sellmeier"); }
    const QIcon& icon() const { static QIcon icon(":/icon/glass_red"); return icon; }
    QStringList coeffNames() const { static QStringList c(
        {"b_1", "b_2", "b_3", "c_1", "c_2", "c_3"}); return c; }
    Glass* makeGlass();
};

class ReznikFormula : public DispersionFormula
{
public:
    const char* name() const { return QT_TRANSLATE_NOOP("Formula", "Reznik"); }
    const QIcon& icon() const { static QIcon icon(":/icon/glass_violet"); return icon; }
    QStringList coeffNames() const { static QStringList c(
        {"c_1", "c_2", "c_3", "c_4", "c_5", "c_6", "c_7", "c_8", "c_9", "c_10", "c_11"}); return c; }
    Glass* makeGlass();
};

class CustomFormula : public DispersionFormula
{
public:
    const char* name() const { return QT_TRANSLATE_NOOP("Formula", "Custom"); }
    const QIcon& icon() const { static QIcon icon(":/icon/glass_blue"); return icon; }
    Glass* makeGlass();
};

inline const QMap<QString, DispersionFormula*>& dispersionFormulas()
{
    static ShottFormula shott;
    static SellmeierFormula sellmeier;
    static ReznikFormula reznik;
    static CustomFormula custom;
    static QMap<QString, DispersionFormula*> formulas {
        { shott.name(), &shott },
        { sellmeier.name(), &sellmeier },
        { reznik.name(), &reznik },
        //{ custom.name(), &custom }
    };
    return formulas;
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
    bool isGlass() const;
    FolderItem* asFolder();
    GlassItem* asGlass();

private:
    int _id;
    QString _title, _info;
    CatalogItem* _parent = nullptr;

    friend class Catalog;
    friend class FolderManager;
    friend class GlassManager;
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

class GlassItem : public CatalogItem
{
public:
    ~GlassItem();

    Glass* glass() const { return _glass; }
    DispersionFormula* formula() { return _formula; }

private:
    Glass* _glass = nullptr;
    DispersionFormula* _formula = nullptr;

    friend class Catalog;
    friend class GlassManager;
};

//------------------------------------------------------------------------------

class Catalog : public QObject
{
    Q_OBJECT

public:
    Catalog();
    ~Catalog();

    static QString fileFilter();
    static CatalorResult open(const QString& fileName);
    static CatalorResult create(const QString& fileName);

    const QString& fileName() const { return _fileName; }
    const QList<CatalogItem*>& items() const { return _items; }

    IntResult countGlasses() const;

    QString renameFolder(FolderItem* item, const QString& title);
    QString createFolder(FolderItem* parent, const QString& title);
    QString removeFolder(FolderItem* item);
    QString createGlass(FolderItem* parent, Glass *glass);
    QString updateGlass(GlassItem* item, Glass* glass);
    QString removeGlass(GlassItem* item);
    QString loadGlass(GlassItem* item);

signals:
    void glassCreated(GlassItem*);
    void glassRemoved(GlassItem*);

private:
    QString _fileName;
    QList<CatalogItem*> _items;
};

#endif // CATALOG_H

