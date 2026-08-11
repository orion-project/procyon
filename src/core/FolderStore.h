#ifndef FOLDER_STORE_H
#define FOLDER_STORE_H

#include <QString>
#include <QMap>

class Folder;

struct FoldersResult
{
    QString error;

    struct Item { int parentId; Folder* folder; };
    QList<Item> items;
};

class FolderStore
{
public:
    QString prepare();

    QString create(Folder* folder) const;
    QString rename(int folderId, const QString title) const;
    QString remove(Folder* folder) const;
    FoldersResult selectAll() const;

private:
    QString removeBranch(Folder* folder, const QString &path) const;
};

namespace Store
{
FolderStore* folders();
}

#endif // FOLDER_STORE_H
