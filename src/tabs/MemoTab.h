#ifndef MEMO_TAB_H
#define MEMO_TAB_H

#include <QWidget>

class Db;
class MemoItem;

class MemoTab : public QWidget
{
    Q_OBJECT

public:
    MemoItem* memoItem() const { return _memoItem; }

    virtual void loadSettings() {}
    virtual bool canClose() { return true; }
    virtual bool isReadOnly() const { return true; }
    virtual bool isModified() const { return false; }
    virtual void beginEdit() {}

signals:
    bool onAboutToBeClosed();
    void onReadOnly(bool readOnly);
    void onModified(bool modified);

protected:
    Db* _db;
    MemoItem* _memoItem;

    explicit MemoTab(Db* db, MemoItem* memoItem);
};

#endif // MEMO_TAB_H
