#ifndef PAGE_WIDGETS_H
#define PAGE_WIDGETS_H

#include <QLineEdit>
#include <QFrame>
#include <QTextEdit>

#include "helpers/OriLayouts.h"

namespace PageWidgets
{

QLineEdit* makeTitleEditor();
QFrame* makeHeaderPanel(Ori::Layouts::LayoutItems items);

} // namespace PageWidgets

#endif // PAGE_WIDGETS_H
