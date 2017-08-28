#include "Appearance.h"
#include "Catalog.h"
#include "Memo.h"
#include "MemoWindow.h"
#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriWidgets.h"

#include <QIcon>
#include <QDebug>
#include <QLineEdit>
#include <QTextEdit>
#include <QToolBar>
#include <QPushButton>

using namespace Ori::Layouts;

//------------------------------------------------------------------------------

MemoWindow::MemoWindow(Catalog *catalog, MemoItem *memoItem) : QWidget(),
    _catalog(catalog), _memoItem(memoItem)
{
    setWindowIcon(QIcon(":/icon/memo_plain_text"));

    connect(catalog, &Catalog::memoRemoved, this, &MemoWindow::memoRemoved);

    _memoEditor = new QTextEdit;
    _memoEditor->setReadOnly(true);
    QFont f = _memoEditor->font();
    f.setFamily("Arial");
    f.setPointSize(12);
    _memoEditor->setFont(f);
    _memoEditor->setAcceptRichText(false);


    _titleEditor = new QLineEdit;
    f = _titleEditor->font();
    f.setFamily("Arial");
    f.setPointSize(14);
    _titleEditor->setFont(f);


    auto toolbar = new QToolBar;
    _actionEdit = toolbar->addAction(QIcon(":/toolbar/memo_edit"), tr("Edit"), this, &MemoWindow::beginEditing);
    _actionSave = toolbar->addAction(QIcon(":/toolbar/memo_save"), tr("Save"), this, &MemoWindow::saveEditing);
    _actionCancel = toolbar->addAction(QIcon(":/toolbar/memo_cancel"), tr("Cancel"), this, &MemoWindow::cancelEditing);

    auto toolPanel = LayoutH({_titleEditor, toolbar})
        .setMargin(0)
        .makeWidget();

    LayoutV({toolPanel, _memoEditor}).setMargin(0).setSpacing(0).useFor(this);

    showMemo();
    toggleEditMode(false);
    _memoEditor->setFocus();
}

MemoWindow::~MemoWindow()
{
}

void MemoWindow::memoRemoved(MemoItem* item)
{
    if (item == _memoItem)
        deleteLater();
}

void MemoWindow::showMemo()
{
    _memoEditor->setPlainText(_memoItem->memo()->data());
    _titleEditor->setText(_memoItem->memo()->title());
    setWindowTitle(_memoItem->memo()->title());
}

void MemoWindow::beginEditing()
{
    toggleEditMode(true);
}

void MemoWindow::cancelEditing()
{
    toggleEditMode(false);
    showMemo();
}

void MemoWindow::saveEditing()
{
    auto memo = _memoItem->type()->makeMemo();
    // TODO preserve additional non editable data - dates, etc.
    memo->_id = _memoItem->memo()->id();
    memo->_title  = _titleEditor->text().trimmed();
    memo->_data = _memoEditor->toPlainText();

    auto res = _catalog->updateMemo(_memoItem, memo);
    if (!res.isEmpty())
        return Ori::Dlg::error(res);

    setWindowTitle(_memoItem->memo()->title());

    toggleEditMode(false);
}

void MemoWindow::toggleEditMode(bool on)
{
    _actionSave->setVisible(on);
    _actionCancel->setVisible(on);
    _actionEdit->setVisible(!on);
    _memoEditor->setReadOnly(!on);
    _titleEditor->setReadOnly(!on);
    _titleEditor->setStyleSheet(QString("border-style: none; background: %1; padding: 6px")
        .arg(palette().color(on ? QPalette::Base : QPalette::Window).name()));
}
