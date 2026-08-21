#include "IssueMemoTab.h"

#include "TabHelpers.h"
#include "core/Enot.h"
#include "core/MemoStore.h"
#include "widgets/MemoPropsPanel.h"

#include <QAction>
#include <QLabel>
#include <QLineEdit>
#include <QToolBar>

typedef IssueMemoTab Self;  

IssueMemoTab::IssueMemoTab(Enot* enot, Memo* memo) : MemoTab(enot, memo)
{
    auto idLabel = new QLabel('#' + QString::number(memo->id()));
    idLabel->setObjectName("issue_id");
    
    _titleEditor = TabHelpers::makeTitleEditor();
    
    _toolbar = TabHelpers::makeHeaderToolBar();

    _actionEdit = _toolbar->addAction(QIcon(":/toolbar/edit"), tr("Edit"), this, &Self::beginEdit);
    _actionSave = _toolbar->addAction(QIcon(":/toolbar/apply"), tr("Save"), this, &Self::saveEdit);
    _actionCancel = _toolbar->addAction(QIcon(":/toolbar/cancel"), tr("Cancel"), this, &Self::cancelEdit);
    _actionEdit->setShortcut(QKeySequence(Qt::Key_Return, Qt::Key_Return));
    _actionSave->setShortcut(QKeySequence::Save);
    _actionCancel->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_Escape));
    _toolbar->addSeparator();
    _toolbar->addAction(QIcon(":/toolbar/close"), tr("Close Tab"), [this](){ deleteLater(); });
    
    auto toolPanel = TabHelpers::makeHeaderPanel({idLabel, _titleEditor, _toolbar});

    _propsPanel = new MemoPropsPanel(enot);
    _propsPanel->setVisible(false);

    Ori::Layouts::LayoutV({toolPanel, _propsPanel, Ori::Layouts::Stretch()}).setMargin(0).setSpacing(0).useFor(this);

    const auto& props = memo->props();
    for (auto it = props.cbegin(); it != props.cend(); it++)
        _propsPanel->addProp(it.key(), it.value());

    showMemo();
    toggleEditMode(false);
}

void IssueMemoTab::showMemo()
{
    _titleEditor->setText(_memo->title());
    
    setWindowTitle(_memo->title());
}

void IssueMemoTab::beginEdit()
{
    toggleEditMode(true);

    _titleEditor->setFocus();
    _titleEditor->selectAll();
}

void IssueMemoTab::cancelEdit()
{
    toggleEditMode(false);
    _titleEditor->setText(_memo->title());
}

bool IssueMemoTab::saveEdit()
{
    _propsPanel->apply();

    MemoUpdateParam update;
    QString newTitle = _titleEditor->text().trimmed();
    if (newTitle != _memo->title())
        update.title = newTitle;
    if (_propsPanel->hasValues())
        update.props = _propsPanel->values();

    auto ok = _enot->updateMemo(_memo, update);
    if (!ok) return false;

    setWindowTitle(_memo->title());
    toggleEditMode(false);
    return true;
}

void IssueMemoTab::toggleEditMode(bool on)
{
    _propsPanel->setReadOnly(on);

    _actionSave->setVisible(on);
    _actionCancel->setVisible(on);
    _actionEdit->setVisible(!on);

    TabHelpers::setTitleEditorReadOnly(_titleEditor, !on);
}
