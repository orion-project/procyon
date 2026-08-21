#include "QssEditorTab.h"

#include "TabHelpers.h"
#include "../highlighter/PhlManager.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriTheme.h"
#include "helpers/OriWidgets.h"
#include "widgets/OriCodeEditor.h"
#include "widgets/OriPopupMessage.h"

#include <QApplication>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QSplitter>
#include <QPushButton>
#include <QToolBar>

QssEditorTab::QssEditorTab(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Application QSS Editor");
    setWindowIcon(QIcon(":/icon/main"));

    _editor = new Ori::Widgets::CodeEditor;
    _editor->setPlainText(Ori::Theme::loadRawStyleSheet());
    _editor->setProperty("role", "memo_editor");
    _editor->setObjectName("code_editor");
    Phl::createHighlighter(_editor, "qss");

    auto titleEditor = TabHelpers::makeTitleEditor(windowTitle());

    auto toolbar = new QToolBar;
    auto actionApply = toolbar->addAction(QIcon(":/toolbar/apply"), "Apply", this, [this]{
        qApp->setStyleSheet(Ori::Theme::makeStyleSheet(_editor->toPlainText()));
    });
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/close"), "Close", this, &QssEditorTab::deleteLater);

    actionApply->setShortcut(Qt::Key_F5);

    auto splitter = Ori::Gui::splitterH(_editor, makeToolsPanel());
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    splitter->setSizePolicy(splitter->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);

    auto header = TabHelpers::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({header, splitter}).setMargin(0).setSpacing(0).useFor(this);
}

QWidget* QssEditorTab::makeToolsPanel()
{
    return Ori::Layouts::LayoutV({
        makeWarningBox(),
        makePopupMsgTool(),
        Ori::Layouts::Stretch()
    }).setSpacing(10).makeWidget();
}

QWidget* QssEditorTab::makePopupMsgTool()
{
    auto textEdit = new QLineEdit("This is a popup message text");

    auto buttonAffirm = Ori::Gui::button("Show affirmation", [textEdit]{
        Ori::Gui::PopupMessage::affirm(textEdit->text(), 0);
    });

    auto buttonError = Ori::Gui::button("Show error", [textEdit]{
        Ori::Gui::PopupMessage::error(textEdit->text(), 0);
    });

    auto buttonWarning = Ori::Gui::button("Show warning", [textEdit]{
        Ori::Gui::PopupMessage::warning(textEdit->text(), 0);
    });

    auto buttonHint = Ori::Gui::button("Show hint", [textEdit]{
        Ori::Gui::PopupMessage::hint(textEdit->text(), 0);
    });

    return Ori::Layouts::LayoutV({textEdit, buttonError, buttonWarning, buttonAffirm, buttonHint})
        .makeGroupBox("Test Popup Message");
}

QWidget* QssEditorTab::makeWarningBox()
{
    auto label = new QLabel(
        "Application style sheet can't be persistently saved in runtime, "
        "it only can be changed during compilation. "
        "This tab is only for testing and developing style sheet. "
        "When it's done, the style sheet has to be saved into <code>app.qss</code> file and the app rebuilt");
    label->setWordWrap(true);
    auto button = Ori::Gui::button("Save app.qss", [this]{
        auto res = Ori::Theme::saveRawStyleSheet(_editor->toPlainText());
        if (!res.isEmpty()) Ori::Dlg::error(res);
        else Ori::Gui::PopupMessage::affirm("Saved successfully", 1000);
    });
    return Ori::Layouts::LayoutV({label, button})
        .makeGroupBox("Developer Mode");
}
