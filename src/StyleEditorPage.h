#ifndef STYLE_EDITOR_PAGE_H
#define STYLE_EDITOR_PAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

class StyleEditorPage : public QWidget
{
    Q_OBJECT

public:
    explicit StyleEditorPage(QWidget *parent = nullptr);

private:
    QPlainTextEdit* _editor;
};

#endif // STYLE_EDITOR_PAGE_H
