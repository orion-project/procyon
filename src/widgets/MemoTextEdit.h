#ifndef MEMO_TEXT_EDIT_H
#define MEMO_TEXT_EDIT_H

#include <QTextEdit>

class MemoTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit MemoTextEdit(QWidget* parent = nullptr);

    bool wordWrap() const;
    void setWordWrap(bool on);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    bool event(QEvent *event) override;

private:
    QString _clickedHref;

    QString hyperlinkAt(const QPoint& pos) const;
};

#endif // MEMO_TEXT_EDIT_H
