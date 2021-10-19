#include "QssEditorPage.h"

#include "PageWidgets.h"
#include "../AppTheme.h"
#include "../widgets/CodeTextEdit.h"
#include "../widgets/PopupMessage.h"
#include "orion/helpers/OriDialogs.h"
#include "orion/helpers/OriLayouts.h"
#include "orion/helpers/OriWidgets.h"

#include <QApplication>
#include <QGroupBox>
#include <QIcon>
#include <QLabel>
#include <QSplitter>
#include <QPushButton>
#include <QToolBar>

QssEditorPage::QssEditorPage(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Application QSS Editor");
    setWindowIcon(QIcon(":/icon/main"));

    _editor = new CodeTextEdit("qss");
    _editor->setPlainText(AppTheme::loadRawStyleSheet());

    auto titleEditor = PageWidgets::makeTitleEditor(windowTitle());

    auto toolbar = new QToolBar;
    auto actionApply = toolbar->addAction(QIcon(":/toolbar/apply"), "Apply", this, [this]{
        qApp->setStyleSheet(AppTheme::makeStyleSheet(_editor->toPlainText()));
    });
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/close"), "Close", this, &QssEditorPage::deleteLater);

    actionApply->setShortcut(Qt::Key_F5);

    auto splitter = Ori::Gui::splitterH(_editor, makeToolsPanel());
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    splitter->setSizePolicy(splitter->sizePolicy().horizontalPolicy(), QSizePolicy::Expanding);

    auto header = PageWidgets::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({header, splitter}).setMargin(0).setSpacing(0).useFor(this);
}

QWidget* QssEditorPage::makeToolsPanel()
{
    return Ori::Layouts::LayoutV({
        makeWarningBox(),
        makePopupMsgTool(),
        Ori::Layouts::Stretch()
    }).setSpacing(10).makeWidget();
}

QWidget* QssEditorPage::makePopupMsgTool()
{
    auto textEdit = new QLineEdit("This is a popup message text");

    auto buttonAffirm = new QPushButton("Show affirmation");
    connect(buttonAffirm, &QPushButton::clicked, this, [textEdit]{
        PopupMessage::affirm(textEdit->text(), 0);
    });

    auto buttonError = new QPushButton("Show error");
    connect(buttonError, &QPushButton::clicked, this, [textEdit]{
        PopupMessage::error(textEdit->text(), 0);
    });

    return Ori::Gui::groupV("Test Popup Message", {textEdit, buttonAffirm, buttonError});
}

QWidget* QssEditorPage::makeWarningBox()
{
    auto label = new QLabel(
        "Application style sheet can't be persistently saved in runtime, "
        "it only can be changed during compilation. "
        "This page is only for testing and developing style sheet. "
        "When it's done, the style sheet has to be saved into <code>app.qss</code> file and the app rebuilt");
    label->setWordWrap(true);
    auto button = new QPushButton("Save app.qss");
    connect(button, &QPushButton::clicked, this, [this]{
        auto res = AppTheme::saveRawStyleSheet(_editor->toPlainText());
        if (!res.isEmpty()) Ori::Dlg::error(res);
        else PopupMessage::affirm("Saved successfully", 1000);
    });
    return Ori::Gui::groupV("Developer Mode", {label, button});
}
