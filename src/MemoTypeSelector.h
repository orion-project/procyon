#ifndef MEMOTYPESELECTOR_H
#define MEMOTYPESELECTOR_H

#include <QDialog>

class MemoType;

class MemoTypeSelector : public QDialog
{
    Q_OBJECT

public:
    explicit MemoTypeSelector(QWidget *parent = nullptr);

    static MemoType* selectType();
};

#endif // MEMOTYPESELECTOR_H
