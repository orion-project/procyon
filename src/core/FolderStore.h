#ifndef FOLDER_STORE_H
#define FOLDER_STORE_H

#include <QString>
#include <QMap>

class FolderItem;

struct FoldersResult
{
    QString error;

    struct Item { int parentId; FolderItem* folder; };
    QList<Item> items;
};

class FolderStore
{
public:
    QString prepare();

    QString create(FolderItem* folder) const;
    QString rename(int folderId, const QString title) const;
    QString remove(FolderItem* folder) const;
    FoldersResult selectAll() const;

private:
    QString removeBranch(FolderItem* folder, const QString &path) const;
};

namespace Store
{
FolderStore* folders();
}

#endif // FOLDER_STORE_H
