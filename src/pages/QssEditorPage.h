#ifndef QSS_EDITOR_PAGE_H
#define QSS_EDITOR_PAGE_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

class QssEditorPage : public QWidget
{
    Q_OBJECT

public:
    explicit QssEditorPage(QWidget *parent = nullptr);

private:
    QPlainTextEdit *_editor;

    QWidget* makeToolsPanel();
    QWidget* makePopupMsgTool();
    QWidget* makeWarningBox();
};

#endif // QSS_EDITOR_PAGE_H
