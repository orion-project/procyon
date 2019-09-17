#ifndef MEMO_EDITOR_H
#define MEMO_EDITOR_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTextEdit;
QT_END_NAMESPACE

class MemoItem;
class TextEditSpellcheck;

class MemoEditor : public QWidget
{
    Q_OBJECT

public:
    virtual void setFocus() {}
    virtual void setFont(const QFont&) {}
    virtual void setUnmodified() = 0;
    virtual bool isModified() const = 0;
    virtual void setReadOnly(bool on) = 0;
    virtual bool isReadOnly() const = 0;
    virtual void setWordWrap(bool on) = 0;
    virtual void showMemo(MemoItem*) = 0;
    virtual QString data() const = 0;
    virtual void toggleSpellcheck(bool) {}
    virtual void setSpellcheckLang(const QString&) {}
    virtual QString spellcheckLang() const { return QString(); }

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
    void setFont(const QFont& f) override;
    void setUnmodified() override;
    bool isModified() const override;
    void setReadOnly(bool on) override;
    bool isReadOnly() const override;
    void setWordWrap(bool on) override;
    QString data() const override;
    void toggleSpellcheck(bool on) override;
    void setSpellcheckLang(const QString& lang) override;
    QString spellcheckLang() const override { return _spellcheckLang; }

protected:
    explicit TextMemoEditor(MemoItem* memoItem, QWidget *parent = nullptr);

    QTextEdit* _editor;
    TextEditSpellcheck* _spellcheck = nullptr;
    QString _spellcheckLang;

    void setEditor(QTextEdit*);
};

#endif // MEMO_EDITOR_H
