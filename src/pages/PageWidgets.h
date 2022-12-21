#ifndef PAGE_WIDGETS_H
#define PAGE_WIDGETS_H

#include "helpers/OriLayouts.h"

#include <QFrame>
#include <QLineEdit>

namespace PageWidgets
{

QLineEdit* makeTitleEditor(const QString& title = QString());
QFrame* makeHeaderPanel(Ori::Layouts::LayoutItems items);
QString formatInfo(const QString& info);
QString formatError(const QString& msg);

} // namespace PageWidgets

#endif // PAGE_WIDGETS_H
