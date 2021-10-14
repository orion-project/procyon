#include "OriHighlighterEditor.h"
#include "OriHighlighter.h"

#include "orion/helpers/OriLayouts.h"

#include <QLabel>
#include <QPlainTextEdit>

using namespace Ori::Layouts;

namespace Ori {
namespace Highlighter {

void createHighlighterDlg(const QSharedPointer<SpecStorage>& storage, const QSharedPointer<Spec> &base)
{
    EditorDialog::create(storage, base)->show();
}

void editHighlighterDlg(const QSharedPointer<Spec> &spec)
{
    EditorDialog::edit(spec)->show();
}

EditorDialog* EditorDialog::create(const QSharedPointer<SpecStorage>& storage, const QSharedPointer<Spec>& base)
{
    auto dlg = new EditorDialog;
    dlg->setWindowTitle(tr("Create Highlighter"));
    dlg->_storage = storage;
    if (base)
        dlg->_spec = base->meta.storage->loadSpec(base->meta.source, true);
    else
        dlg->_spec.reset(new Spec);
    dlg->createWidgets();
    return dlg;
}

EditorDialog* EditorDialog::edit(const QSharedPointer<Spec>& spec)
{
    auto dlg = new EditorDialog;
    dlg->setWindowTitle(tr("Edit Highlighter: %1").arg(spec->meta.displayTitle()));
    dlg->_target = spec;
    dlg->_storage = spec->meta.storage;
    dlg->_spec = spec->meta.storage->loadSpec(spec->meta.source, true);
    dlg->createWidgets();
    return dlg;
}

EditorDialog::EditorDialog() : QWidget()
{
    setAttribute(Qt::WA_DeleteOnClose);
}

void EditorDialog::createWidgets()
{
    _code = new QPlainTextEdit;
    _code->setPlainText(_spec->code);

    _sample = new QPlainTextEdit;
    _sample->setPlainText(_spec->sample);
    new Highlighter(_sample->document(), _spec);

    setLayout(LayoutH({
                          LayoutV({
                              new QLabel(tr("Code")),
                              _code,
                          }),
                          LayoutV({
                              new QLabel(tr("Example")),
                              _sample,
                          }),
                      }).boxLayout());
}

} // namespace Highlighter
} // namespace Ori
