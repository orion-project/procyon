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
    Ori::Gui::setFontMonospace(_memoEditor);

    _titleEditor = new QLineEdit;

    _buttonEdit = new QPushButton(tr("Edit"));
    _buttonSave = new QPushButton(tr("Save"));
    _buttonCancel = new QPushButton(tr("Cancel"));
    connect(_buttonEdit, &QPushButton::clicked, this, &MemoWindow::beginEditing);
    connect(_buttonSave, &QPushButton::clicked, this, &MemoWindow::saveEditing);
    connect(_buttonCancel, &QPushButton::clicked, this, &MemoWindow::cancelEditing);

    auto toolbar = LayoutH({_titleEditor, Stretch(), _buttonEdit, _buttonSave, _buttonCancel}).makeWidget();

    LayoutV({toolbar, _memoEditor}).setMargin(0).setSpacing(0).useFor(this);

    toggleEditMode(false);
    showMemo();
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

    toggleEditMode(false);
}

void MemoWindow::toggleEditMode(bool on)
{
    _buttonSave->setVisible(on);
    _buttonCancel->setVisible(on);
    _buttonEdit->setVisible(!on);
    _memoEditor->setReadOnly(!on);
    _titleEditor->setReadOnly(!on);
}
