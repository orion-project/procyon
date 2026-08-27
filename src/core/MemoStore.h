#ifndef MEMO_STORE_H
#define MEMO_STORE_H

#include <QString>
#include <QHash>
#include <QVariant>

class Memo;
class MemoEvent;
class MemoSheet;
struct MemoUpdateParam;

struct MemosResult
{
    QString error;
    QStringList warnings;

    struct Item { int folderId; Memo* memo; };
    QList<Item> items;
};

class MemoStore
{
public:
    QString prepare();

    QString create(Memo* memo) const;
    QString update(Memo *memo, const MemoUpdateParam& update) const;
    QString remove(Memo* memo) const;
    QString load(Memo *memo) const;
    MemosResult selectAll() const;
    QString countAll(int* count) const;
    QHash<QString, QVariant> selectOptions(int memoId) const;
    QString updateOption(int memoId, const QString& name, const QVariant& value) const;
    QHash<QString, QString> loadProps(int memoId) const;
    QStringList loadPropNames() const;
    QStringList loadPropValues(const QString& name) const;
    QString deleteProp(int memoId, const QString& name) const;
    QString updateProp(int memoId, const QString& name, const QString& value) const;
    QList<MemoSheet> loadSheets(int memoId) const;
    QString addSheet(int memoId, const QString& text) const;
    QString updateSheet(int sheetId, const QString& text) const;
    QList<MemoEvent> loadEvents(int memoId) const;
    void writeEvent(const MemoEvent& event) const;
    
private:
    QString _station;
};

namespace Store
{
MemoStore* memos();
}

#endif // MEMO_STORE_H
