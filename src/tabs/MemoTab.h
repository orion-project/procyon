#ifndef MEMO_TAB_H
#define MEMO_TAB_H

#include <QWidget>

class Enot;
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
    Enot* _enot;
    MemoItem* _memoItem;

    explicit MemoTab(Enot* enot, MemoItem* memoItem);
};

#endif // MEMO_TAB_H
