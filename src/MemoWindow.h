#ifndef DISPERSIONPLOT_H
#define DISPERSIONPLOT_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTextEdit;
QT_END_NAMESPACE

class Catalog;
class MemoItem;

class MemoWindow : public QWidget
{
    Q_OBJECT

public:
    explicit MemoWindow(Catalog* catalog, MemoItem* memo);
    ~MemoWindow();

private:
    MemoItem* _memo;
    QTextEdit* _memoEditor;

    void memoRemoved(MemoItem* item);
};

#endif // DISPERSIONPLOT_H
