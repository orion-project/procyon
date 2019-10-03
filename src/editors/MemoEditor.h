#ifndef MEMO_EDITOR_H
#define MEMO_EDITOR_H

#include <QWidget>

class MemoItem;
class MemoTextEdit;
class TextEditSpellcheck;

// TODO: all text related options (font, word-wrap, etc.) should be removed
// from base edior class when non-text memo types will happen
class MemoEditor : public QWidget
{
    Q_OBJECT

public:
    virtual void setFocus() = 0;
    virtual QFont font() const = 0;
    virtual void setFont(const QFont&) = 0;
    virtual bool isModified() const = 0;
    virtual bool wordWrap() const = 0;
    virtual void setWordWrap(bool on) = 0;
    virtual void showMemo() = 0;
    virtual QString data() const = 0;
    virtual void setSpellcheckLang(const QString&) = 0;
    virtual QString spellcheckLang() const = 0;
    virtual void beginEdit() = 0;
    virtual void endEdit() = 0;
    virtual void saveEdit() = 0;

signals:
    void onModified(bool modified);

protected:
    explicit MemoEditor(MemoItem* memoItem, QWidget *parent = nullptr);

    MemoItem* _memoItem;
};


class TextMemoEditor : public MemoEditor
{
    Q_OBJECT

public:
    void setFocus() override;
    QFont font() const override;
    void setFont(const QFont& f) override;
    bool isModified() const override;
    bool wordWrap() const override;
    void setWordWrap(bool on) override;
    QString data() const override;
    void setSpellcheckLang(const QString& lang) override;
    QString spellcheckLang() const override { return _spellcheckLang; }
    void beginEdit() override;
    void endEdit() override;
    void saveEdit() override { endEdit(); }

protected:
    explicit TextMemoEditor(MemoItem* memoItem, QWidget *parent = nullptr);

    MemoTextEdit* _editor = nullptr;
    TextEditSpellcheck* _spellcheck = nullptr;
    QString _spellcheckLang;

    void setEditor(MemoTextEdit*);
    void setReadOnly(bool on);
    void toggleSpellcheck(bool on);
};

#endif // MEMO_EDITOR_H
