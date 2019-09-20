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
    virtual bool isModified() const = 0;
    virtual void setWordWrap(bool on) = 0;
    virtual void showMemo() = 0;
    virtual QString data() const = 0;
    virtual void setSpellcheckLang(const QString&) {}
    virtual QString spellcheckLang() const { return QString(); }
    virtual void beginEdit() {}
    virtual void endEdit() {}
    virtual void saveEdit() {}

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
    bool isModified() const override;
    void setWordWrap(bool on) override;
    QString data() const override;
    void setSpellcheckLang(const QString& lang) override;
    QString spellcheckLang() const override { return _spellcheckLang; }
    void beginEdit() override;
    void endEdit() override;
    void saveEdit() override { endEdit(); }

protected:
    explicit TextMemoEditor(MemoItem* memoItem, QWidget *parent = nullptr);

    QTextEdit* _editor = nullptr;
    TextEditSpellcheck* _spellcheck = nullptr;
    QString _spellcheckLang;

    void setEditor(QTextEdit*);
    void setReadOnly(bool on);
    void toggleSpellcheck(bool on);
};

#endif // MEMO_EDITOR_H
