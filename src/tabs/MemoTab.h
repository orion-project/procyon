#ifndef MEMO_TAB_H
#define MEMO_TAB_H

#include <QWidget>

class Enot;
class Memo;

class MemoTab : public QWidget
{
    Q_OBJECT

public:
    Memo* memo() const { return _memo; }

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
    Memo* _memo;

    explicit MemoTab(Enot* enot, Memo* memo);
};

#endif // MEMO_TAB_H
