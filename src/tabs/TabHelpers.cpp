#include "TabHelpers.h"

#include <QFrame>
#include <QLineEdit>
#include <QToolBar>

namespace TabHelpers
{

QToolBar* makeHeaderToolBar()
{
    auto toolBar = new QToolBar;
    toolBar->setObjectName("memo_toolbar");
    toolBar->setContentsMargins(0, 0, 0, 0);
    toolBar->setIconSize(QSize(24, 24));
    return toolBar;
}

QLineEdit* makeTitleEditor(const QString &title)
{
    auto titleEditor = new QLineEdit;
    titleEditor->setReadOnly(true);
    titleEditor->setObjectName("memo_title_editor");
    titleEditor->setText(title);
    return titleEditor;
}

QFrame* makeHeaderPanel(Ori::Layouts::LayoutItems items)
{
    auto toolPanel = new QFrame;
    toolPanel->setObjectName("memo_header_panel");
    Ori::Layouts::LayoutH(items).setMargin(0).useFor(toolPanel);
    return toolPanel;
}

QString formatInfo(const QString& info)
{
    return "<span style='background:silver;color:white;font-weight:bold'>&nbsp;&nbsp;i&nbsp;&nbsp;</span>&nbsp; " + info;
}

QString formatError(const QString& msg)
{
    return QString("<span style='color:red;white-space:pre'>%1</span>").arg(msg);
}

void setTitleEditorReadOnly(QLineEdit *titleEditor, bool on)
{
    titleEditor->setReadOnly(on);
    // Force updating editor's style sheet, seems it doesn't note changing of readOnly or a custom property
    titleEditor->setStyleSheet(QString("QLineEdit { background: %1 }").arg(on ? "transparent" : "white"));
}

} // namespace TabHelpers
