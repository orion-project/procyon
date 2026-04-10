#ifndef QSS_EDITOR_TAB_H
#define QSS_EDITOR_TAB_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

class QssEditorTab : public QWidget
{
    Q_OBJECT

public:
    explicit QssEditorTab(QWidget *parent = nullptr);

private:
    QPlainTextEdit *_editor;

    QWidget* makeToolsPanel();
    QWidget* makePopupMsgTool();
    QWidget* makeWarningBox();
};

#endif // QSS_EDITOR_TAB_H
