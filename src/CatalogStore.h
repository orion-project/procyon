#ifndef CATALOGSTORE_H
#define CATALOGSTORE_H

#include <QVector>

class CatalogItem;
class GlassItem;
class FolderItem;
class FolderTableDef;
class GlassTableDef;
class Glass;

//------------------------------------------------------------------------------

struct FoldersResult
{
    QString error;
    QMap<int, FolderItem*> items;
};

class FolderManager
{
public:
    FolderTableDef* table() const;

    QString create(FolderItem* folder) const;
    QString rename(int folderId, const QString title) const;
    QString remove(FolderItem* folder) const;
    FoldersResult selectAll() const;

private:
    QString removeBranch(FolderItem* folder, const QString &path) const;
};

//------------------------------------------------------------------------------

struct GlassesResult
{
    QString error;
    QStringList warnings;
    QMap<int, QList<GlassItem*>> items;
};

class GlassManager
{
public:
    GlassTableDef* table() const;

    QString create(GlassItem* item) const;
    QString update(Glass* glass, const QString& info) const;
    QString remove(GlassItem* item) const;
    QString load(Glass* glass) const;
    GlassesResult selectAll() const;
    QString countAll(int* count) const;
};

//------------------------------------------------------------------------------

namespace CatalogStore {

void closeDatabase();
QString newDatabase(const QString fileName);
QString openDatabase(const QString fileName);
QString beginTran(const QString& operation);
void commitTran();
void rollbackTran();

FolderManager* folderManager();
GlassManager* glassManager();

} // namespace CatalogStore

#endif // CATALOGSTORE_H
