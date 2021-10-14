#include "AppSettingsPage.h"

#include "PageWidgets.h"
#include "helpers/OriLayouts.h"

#include <QDebug>
#include <QListWidget>
#include <QScrollArea>
#include <QToolBar>

AppSettingsPage::AppSettingsPage(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("Application Settings");
    setWindowIcon(QIcon(":/icon/settings"));

    auto optionsList = new QScrollArea;
    optionsList->setObjectName("settings_options_list");

    auto editor = new QFrame;
    editor->setProperty("role", "memo_editor");
    Ori::Layouts::LayoutH({makeCategoriesList(), optionsList}).setMargin(0).useFor(editor);

    auto titleEditor = PageWidgets::makeTitleEditor(windowTitle());

    auto toolbar = new QToolBar;
    toolbar->addAction(QIcon(":/toolbar/apply"), "Apply", [](){
        qDebug() << "Apply settings";
    });
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/close"), "Close", [this](){
        deleteLater();
    });

    auto toolPanel = PageWidgets::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, editor}).setMargin(0).setSpacing(0).useFor(this);
}

QWidget* AppSettingsPage::makeCategoriesList()
{
    auto w = new QListWidget;
    w->setObjectName("settings_category_list");
    w->setFixedWidth(200);
    w->addItem(tr("All"));
    QSet<QString> categories;
    for (auto option : _options.items())
        if (!categories.contains(option->category))
        {
            w->addItem(option->category);
            categories << option->category;
        }
    w->setCurrentRow(0);
    return w;
}
