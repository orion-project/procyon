#ifndef MEMO_TYPE_H
#define MEMO_TYPE_H

#include <QString>
#include <QIcon>

class MemoType
{
public:
    QString name() const { return _name; }
    const char* title() const { return _title; }
    const QIcon& icon() const { return _icon; }

    static MemoType* plainText();
    static MemoType* markdown();
    static MemoType* richText();

    static const QList<MemoType*>& all();

    static MemoType* findByName(const QString& name);

    static MemoType* selectFromDlg();

private:
    MemoType(const QString& name, const char* title, const QString& iconPath);

    QString _name;
    const char* _title;
    QIcon _icon;
};

#endif // MEMO_TYPE_H