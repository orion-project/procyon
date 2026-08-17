#ifndef TAB_HELPERS_H
#define TAB_HELPERS_H

#include "helpers/OriLayouts.h"

QT_BEGIN_NAMESPACE
class QFrame;
class QLineEdit;
class QToolBar;
QT_END_NAMESPACE

namespace TabHelpers
{

QToolBar* makeHeaderToolBar();
QLineEdit* makeTitleEditor(const QString& title = QString());
QFrame* makeHeaderPanel(Ori::Layouts::LayoutItems items);

QString formatInfo(const QString& info);
QString formatError(const QString& msg);

void setTitleEditorReadOnly(QLineEdit *titleEditor, bool on);

} // namespace TabHelpers

#endif // TAB_HELPERS_H
