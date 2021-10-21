#include "PageWidgets.h"

namespace PageWidgets
{

QLineEdit* makeTitleEditor(const QString &title)
{
    auto titleEditor = new QLineEdit;
    titleEditor->setReadOnly(true);
    titleEditor->setObjectName("memo_title_editor");
    titleEditor->setProperty("role", "memo_title");
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

} // namespace PageWidgets
