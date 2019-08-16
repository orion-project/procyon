#ifndef MEMO_EDITOR_H
#define MEMO_EDITOR_H

#include <QTextEdit>

class Spellchecker;

class MemoEditor : public QTextEdit
{
    Q_OBJECT

public:
    void setSpellchecker(Spellchecker* checker);

protected:
    void contextMenuEvent(QContextMenuEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    bool event(QEvent *event) override;

private:
    Spellchecker* _spellchecker = nullptr;
    QString _clickedHref;
    QString hyperlinkAt(const QPoint& pos) const;
    QTextCursor spellingAt(const QPoint& pos) const;
    void showSpellcheckMenu(QTextCursor& cursor, const QPoint& pos);
};

#endif // MEMO_EDITOR_H
