#ifndef MEMO_H
#define MEMO_H

#include <QString>
#include <QMap>

class MemoType;

class Memo
{
public:
    virtual ~Memo() {}

    int id() const { return _id; }
    const QString& title() const { return _title; }
    const QString& data() const { return _data; }
    MemoType* type() const { return _type; }

protected:
    Memo(MemoType* type) : _type(type) {}

private:
    int _id;
    QString _title, _data;
    MemoType* _type;

    friend class Catalog;
    friend class MemoManager;
};


class PlainTextMemo : public Memo
{
public:

private:
    PlainTextMemo(MemoType* type) : Memo(type) {}
    friend class PlainTextMemoType;
};


class WikiTextMemo : public Memo
{
public:

private:
    WikiTextMemo(MemoType* type) : Memo(type) {}
    friend class WikiTextMemoType;
};


class RichTextMemo : public Memo
{
public:

private:
    RichTextMemo(MemoType* type) : Memo(type) {}
    friend class RichTextMemoType;
};

#endif // MEMO_H

