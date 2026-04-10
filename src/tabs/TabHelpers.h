#ifndef TAB_HELPERS_H
#define TAB_HELPERS_H

#include "helpers/OriLayouts.h"

#include <QFrame>
#include <QLineEdit>

namespace TabHelpers
{

QLineEdit* makeTitleEditor(const QString& title = QString());
QFrame* makeHeaderPanel(Ori::Layouts::LayoutItems items);
QString formatInfo(const QString& info);
QString formatError(const QString& msg);

} // namespace TabHelpers

#endif // TAB_HELPERS_H
