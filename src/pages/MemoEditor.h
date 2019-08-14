#ifndef MEMO_EDITOR_H
#define MEMO_EDITOR_H

#include <QTextEdit>

class MemoEditor : public QTextEdit
{
    Q_OBJECT

protected:
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    bool event(QEvent *event);

private:
    QString _clickedHref;
    bool shouldProcess(QMouseEvent *e);
    QString hrefAtWidgetPos(const QPoint& pos) const;
};

#endif // MEMO_EDITOR_H
