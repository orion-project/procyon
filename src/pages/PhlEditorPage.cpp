#include "PhlEditorPage.h"
#include "PageWidgets.h"
#include "../AppSettings.h"
#include "../highlighter/OriHighlighter.h"
#include "../widgets/CodeTextEdit.h"
#include "../widgets/PopupMessage.h"

#include "orion/helpers/OriDialogs.h"

#include <QIcon>
#include <QSplitter>
#include <QToolBar>

PhlEditorPage::PhlEditorPage(const QSharedPointer<Ori::Highlighter::Spec>& spec) : QWidget(), spec(spec)
{
    if (spec->meta.name.isEmpty())
        setWindowTitle(tr("Create Highlighter"));
    else
        setWindowTitle(tr("Edit Highlighter: %1").arg(spec->meta.displayTitle()));
    setWindowIcon(QIcon(":/icon/main"));

    _editor = new CodeTextEdit("highlighter");
    _editor->setPlainText(spec->rawCode());

    _sample = new QPlainTextEdit;
    _sample->setWordWrapMode(QTextOption::NoWrap);
    _sample->setProperty("role", "memo_editor");
    _sample->setObjectName("code_editor");
    _sample->setFont(AppSettings::instance().memoFont);
    _sample->setPlainText(spec->rawSample());
    _highlight = new Ori::Highlighter::Highlighter(_sample->document(), spec);

    auto splitter = new QSplitter;
    splitter->addWidget(_editor);
    splitter->addWidget(_sample);
    splitter->setSizePolicy(splitter->sizePolicy().horizontalPolicy(),
                            QSizePolicy::Expanding);

    auto titleEditor = PageWidgets::makeTitleEditor(windowTitle());

    auto toolbar = new QToolBar;
    auto actionCheck = toolbar->addAction(QIcon(":/toolbar/apply"), tr("Check"), this, &PhlEditorPage::checkHighlighter);
    auto actionApply = toolbar->addAction(QIcon(":/toolbar/save"), tr("Save"), this, &PhlEditorPage::saveHighlighter);
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/close"), tr("Close"), [this](){
        deleteLater();
    });

    actionCheck->setShortcut(Qt::Key_F5);
    actionApply->setShortcut(QKeySequence::Save);

    auto toolPanel = PageWidgets::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, splitter}).setMargin(0).setSpacing(0).useFor(this);
}

void PhlEditorPage::checkHighlighter()
{
    auto code = _editor->toPlainText();
    auto warnings = Ori::Highlighter::loadSpecRaw(spec, QStringLiteral("HighlightEditor"), &code, false);
    _editor->setLineHints(warnings);
    _highlight->rehighlight();
}

void PhlEditorPage::saveHighlighter()
{
    if (!spec->meta.storage)
    {
        Ori::Dlg::error("Highlighter has no storage assigned");
        return;
    }
    auto code = _editor->toPlainText();
    auto warnings = Ori::Highlighter::loadSpecRaw(spec, QStringLiteral("HighlightEditor"), &code, true);
    if (warnings.isEmpty())
    {
        auto existed = Ori::Highlighter::checkDuplicates(spec->meta);
        if (existed.first)
            warnings[spec->rawNameLineNo()] = "Another one with the same name already exists";
        if (existed.second)
            warnings[spec->rawTitleLineNo()] = "Another one with the same title already exists";
    }
    _editor->setLineHints(warnings);
    _highlight->rehighlight();
    if (!warnings.isEmpty())
    {
        PopupMessage::error(tr("There are errors in the highlighter code, fix them before saving"));
        return;
    }
    spec->raw[Ori::Highlighter::Spec::RAW_SAMPLE] = _sample->toPlainText();
    auto err = spec->meta.storage->saveSpec(spec);
    if (!err.isEmpty())
        Ori::Dlg::error(tr("Failed to save highlighter\n\n%1").arg(err));
    else
        PopupMessage::affirm(tr("Highlighter successfully saved\n\n"
            "Application is required to be restarted to reflect changes"));
}
