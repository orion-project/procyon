#ifndef MEMO_TYPE_H
#define MEMO_TYPE_H

#include <QString>
#include <QIcon>

class MemoType
{
public:
    virtual ~MemoType();
    virtual const QString name() const = 0;
    virtual const char* title() const = 0;
    virtual const QIcon& icon() const = 0;
    virtual const QString iconPath() const = 0;

    static MemoType* plainText();
    static MemoType* markdown();
    static MemoType* richText();

    static const QList<MemoType*>& all();

    static MemoType* findByName(const QString& name);

    static MemoType* selectFromDlg();
};

#endif // MEMO_TYPE_H