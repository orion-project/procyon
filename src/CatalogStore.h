#ifndef CATALOGSTORE_H
#define CATALOGSTORE_H

#include <QVector>
#include <QVariant>

class CatalogItem;
class MemoItem;
class FolderItem;
class FolderTableDef;
class MemoTableDef;
class Memo;
class SettingsTableDef;

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

struct MemosResult
{
    QString error;
    QStringList warnings;

    // folderId -> [memoItems]
    QMap<int, QList<MemoItem*>> items;

    // memoId -> memoItem
    QMap<int, MemoItem*> allMemos;
};

class MemoManager
{
public:
    MemoTableDef* table() const;

    QString create(MemoItem* item) const;
    QString update(Memo* memo, const QString& info) const;
    QString remove(MemoItem* item) const;
    QString load(Memo* memo) const;
    MemosResult selectAll() const;
    QString countAll(int* count) const;
};

//------------------------------------------------------------------------------

class SettingsManager
{
public:
    enum TrackChangesFlag { IgnoreValuesOrder, RespectValuesOrder };

    SettingsTableDef* table() const;

    void writeValue(const QString& id, const QVariant& value) const;
    QVariant readValue(const QString& id, const QVariant& defValue = QVariant(), bool *hasValue = nullptr) const;

    void writeBool(const QString& id, bool value) const;
    bool readBool(const QString& id, bool defValue) const;

    void writeInt(const QString& id, int value) const;
    int readInt(const QString& id, int defValue) const;
    
    void writeIntArray(const QString& id, const QVector<int>& values,
                       TrackChangesFlag trackChangesFlag = IgnoreValuesOrder) const;
    QVector<int> readIntArray(const QString& id) const;
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
MemoManager* memoManager();
SettingsManager* settingsManager();

} // namespace CatalogStore

#endif // CATALOGSTORE_H
