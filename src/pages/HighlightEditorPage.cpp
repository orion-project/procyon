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

    auto editor = new CodeTextEdit;
    editor->setPlainText(spec->code);
    auto hl = Ori::Highlighter::getSpec("highlighter");
    if (hl)
        new Ori::Highlighter::Highlighter(editor->document(), hl);

    auto sample = new QPlainTextEdit;
    sample->setWordWrapMode(QTextOption::NoWrap);
    sample->setProperty("role", "memo_editor");
    sample->setFont(AppSettings::instance().memoFont);
    sample->setPlainText(spec->sample);
    new Ori::Highlighter::Highlighter(sample->document(), spec);

    auto splitter = new QSplitter;
    splitter->addWidget(editor);
    splitter->addWidget(sample);
    splitter->setSizePolicy(splitter->sizePolicy().horizontalPolicy(),
                            QSizePolicy::Expanding);

    auto titleEditor = PageWidgets::makeTitleEditor(windowTitle());

    auto toolbar = new QToolBar;
    auto actionCheck = toolbar->addAction(QIcon(":/toolbar/edit"), tr("Check"));
    auto actionApply = toolbar->addAction(QIcon(":/toolbar/apply"), tr("Apply"));
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/close"), tr("Close"), [this](){
        deleteLater();
    });

    actionCheck->setShortcut(Qt::Key_F5);

    auto toolPanel = PageWidgets::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, splitter}).setMargin(0).setSpacing(0).useFor(this);
}
