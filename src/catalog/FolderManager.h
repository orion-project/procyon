#ifndef FOLDER_MANAGER_H
#define FOLDER_MANAGER_H

#include <QString>
#include <QMap>

class FolderItem;

struct FoldersResult
{
    QString error;
    QMap<int, FolderItem*> items;
};

class FolderManager
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

#endif // FOLDER_MANAGER_H
