#ifndef PAGE_WIDGETS_H
#define PAGE_WIDGETS_H

#include "helpers/OriLayouts.h"

#include <QFrame>
#include <QLineEdit>
#include <QTextEdit>
#include <QToolBar>

namespace PageWidgets
{

QLineEdit* makeTitleEditor(const QString& title = QString());
QFrame* makeHeaderPanel(Ori::Layouts::LayoutItems items);
QTextEdit* makeCodeEditor();

} // namespace PageWidgets

#endif // PAGE_WIDGETS_H
