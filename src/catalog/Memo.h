#ifndef MEMO_H
#define MEMO_H

#include <QString>

class MemoType;

class Memo
{
public:
    virtual ~Memo();

    int id() const { return _id; }
    void setId(int id) { _id = id; }

    const QString& title() const { return _title; }
    void setTitle(const QString& title) { _title = title; }

    const QString& data() const { return _data; }
    void setData(const QString& data) { _data = data; }

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
    PlainTextMemo(MemoType* type);
    friend class PlainTextMemoType;
};


class WikiTextMemo : public Memo
{
public:

private:
    WikiTextMemo(MemoType* type);
    friend class WikiTextMemoType;
};


class RichTextMemo : public Memo
{
public:

private:
    RichTextMemo(MemoType* type);
    friend class RichTextMemoType;
};

#endif // MEMO_H

