#include "HighlightEditorPage.h"
#include "PageWidgets.h"
#include "../AppSettings.h"
#include "../highlighter/OriHighlighter.h"
#include "../widgets/CodeTextEdit.h"

#include <QIcon>
#include <QSplitter>
#include <QToolBar>

HighlightEditorPage::HighlightEditorPage(const QSharedPointer<Ori::Highlighter::Spec>& spec) : QWidget(), spec(spec)
{
    if (spec->meta.name.isEmpty())
        setWindowTitle(tr("Create Highlighter"));
    else
        setWindowTitle(tr("Edit Highlighter: %1").arg(spec->meta.displayTitle()));
    setWindowIcon(QIcon(":/icon/main"));

    _editor = new CodeTextEdit;
    _editor->setPlainText(spec->code);
    auto hl = Ori::Highlighter::getSpec("highlighter");
    if (hl)
        new Ori::Highlighter::Highlighter(_editor->document(), hl);

    _sample = new QPlainTextEdit;
    _sample->setWordWrapMode(QTextOption::NoWrap);
    _sample->setProperty("role", "memo_editor");
    _sample->setFont(AppSettings::instance().memoFont);
    _sample->setPlainText(spec->sample);
    new Ori::Highlighter::Highlighter(_sample->document(), spec);

    auto splitter = new QSplitter;
    splitter->addWidget(_editor);
    splitter->addWidget(_sample);
    splitter->setSizePolicy(splitter->sizePolicy().horizontalPolicy(),
                            QSizePolicy::Expanding);

    auto titleEditor = PageWidgets::makeTitleEditor(windowTitle());

    auto toolbar = new QToolBar;
    auto actionCheck = toolbar->addAction(QIcon(":/toolbar/update"), tr("Check"), this, &HighlightEditorPage::checkHighlighter);
    auto actionApply = toolbar->addAction(QIcon(":/toolbar/apply"), tr("Apply"), this, &HighlightEditorPage::applyHighlighter);
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/close"), tr("Close"), [this](){
        deleteLater();
    });

    actionCheck->setShortcut(Qt::Key_F5);
    actionApply->setShortcut(QKeySequence::Save);

    auto toolPanel = PageWidgets::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, splitter}).setMargin(0).setSpacing(0).useFor(this);
}

void HighlightEditorPage::checkHighlighter()
{
    _editor->setLineHints({{3, "something"}, {5, "went"}, {15, "wrong"}});
}

void HighlightEditorPage::applyHighlighter()
{
    _editor->setLineHints({});
}
