#ifndef MEMO_EDITOR_H
#define MEMO_EDITOR_H

#include <QTextEdit>

class MemoEditor : public QTextEdit
{
    Q_OBJECT

public:
    explicit MemoEditor(QWidget* parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    bool event(QEvent *event) override;

private:
    QString _clickedHref;

    QString hyperlinkAt(const QPoint& pos) const;
};

#endif // MEMO_EDITOR_H
