#ifndef PHL_MANAGER_H
#define PHL_MANAGER_H

#include <QWidget>

#include "tools/OriHighlighter.h"

QT_BEGIN_NAMESPACE
class QMenu;
QT_END_NAMESPACE

namespace Phl {

typedef QSharedPointer<Ori::Highlighter::Spec> SpecPtr;

SpecPtr getSpec(const QString& name);
QPair<bool, bool> checkDuplicates(const Ori::Highlighter::Meta& meta);

typedef std::function<void(SpecPtr)> SpecEditRequest;
void showManagerDlg(SpecEditRequest onEdit);

void fillMenu(QMenu *menu, std::function<void(const QString&)> onSelect);

template <class TEditor>
QSyntaxHighlighter* createHighlighter(TEditor* editor, const QString& name)
{
    auto hl = !name.isEmpty() ? getSpec(name) : nullptr;
    return hl ? new Ori::Highlighter::Highlighter(editor->document(), hl) : nullptr;
}

void reset();

} // namespace Phl

#endif // PHL_MANAGER_H
