#include "Appearance.h"
#include "Catalog.h"
#include "Memo.h"
#include "MemoWindow.h"
#include "hl/PythonSyntaxHighlighter.h"
#include "hl/ShellMemoSyntaxHighlighter.h"
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

class MemoEditor : public QTextEdit
{
protected:
//    bool event(QEvent *e) override
//    {
//        qDebug() << "event";
//        return QTextEdit::event(e);
//    }

//    void mouseMoveEvent(QMouseEvent *e) override
//    {
//        QTextEdit::mouseMoveEvent(e);
//        //auto cursor = cursorForPosition(e->pos());
//        //qDebug() << cursor.columnNumber();
//        qDebug() << "move";
//    }

    void mouseReleaseEvent(QMouseEvent *e) override
    {
        qDebug() << e->pos() << anchorAt(e->pos());
        QTextEdit::mouseReleaseEvent(e);
    }
};

//------------------------------------------------------------------------------

MemoWindow::MemoWindow(Catalog *catalog, MemoItem *memoItem) : QWidget(),
    _catalog(catalog), _memoItem(memoItem)
{
    setWindowIcon(QIcon(":/icon/memo_plain_text"));

    _memoEditor = new MemoEditor;
    _memoEditor->setReadOnly(true);
    _memoEditor->setAcceptRichText(false);
    _memoEditor->setWordWrapMode(QTextOption::NoWrap);

    _titleEditor = new QLineEdit;
    _titleEditor->setFont(QFont("Arial", 14));

    auto toolbar = new QToolBar;
    _actionEdit = toolbar->addAction(QIcon(":/toolbar/memo_edit"), tr("Edit"), this, &MemoWindow::beginEditing);
    _actionSave = toolbar->addAction(QIcon(":/toolbar/memo_save"), tr("Save"), this, &MemoWindow::saveEditing);
    _actionCancel = toolbar->addAction(QIcon(":/toolbar/memo_cancel"), tr("Cancel"), this, &MemoWindow::cancelEditing);

    auto toolPanel = LayoutH({_titleEditor, toolbar}).setMargin(0).makeWidget();

    LayoutV({toolPanel, _memoEditor}).setMargin(0).setSpacing(0).useFor(this);

    showMemo();
    toggleEditMode(false);
    _memoEditor->setFocus();
}

MemoWindow::~MemoWindow()
{
}

void MemoWindow::showMemo()
{
    auto text = _memoItem->memo()->data();
    _memoEditor->setPlainText(text);
    _titleEditor->setText(_memoItem->memo()->title());
    setWindowTitle(_memoItem->memo()->title());
    applyHighlighter();
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
    {
        Ori::Dlg::error(res);
        cancelEditing();
        return;
    }

    setWindowTitle(_memoItem->memo()->title());
    applyHighlighter();

    toggleEditMode(false);
}

void MemoWindow::toggleEditMode(bool on)
{
    _actionSave->setVisible(on);
    _actionCancel->setVisible(on);
    _actionEdit->setVisible(!on);

    _memoEditor->setReadOnly(!on);
    Qt::TextInteractionFlags flags = Qt::LinksAccessibleByMouse |
        Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard;
    if (on) flags |= Qt::TextEditable;
    _memoEditor->setTextInteractionFlags(flags);

    _titleEditor->setReadOnly(!on);
    _titleEditor->setStyleSheet(QString("QLineEdit { border-style: none; background: %1; padding: 6px }")
        .arg(palette().color(on ? QPalette::Base : QPalette::Window).name()));
}

void MemoWindow::setMemoFont(const QFont& font)
{
    _memoEditor->setFont(font);
}

void MemoWindow::setTitleFont(const QFont& font)
{
    _titleEditor->setFont(font);
}

void MemoWindow::applyHighlighter()
{
    // TODO preserve highlighter if its type is not changed
    if (_highlighter)
    {
        delete _highlighter;
        _highlighter = nullptr;
    }

    auto text = _memoItem->memo()->data();

    // TODO highlighter should be selected bu user and saved into catalog
    if (text.startsWith("#!/usr/bin/env python"))
        _highlighter = new PythonSyntaxHighlighter(_memoEditor->document());
    else if (text.startsWith("#shell-memo"))
        _highlighter = new ShellMemoSyntaxHighlighter(_memoEditor->document());
}
