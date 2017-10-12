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
//#include <QMdiSubWindow>
#include <QTextEdit>
#include <QToolBar>
#include <QDesktopServices>

using namespace Ori::Layouts;

class MemoEditor : public QTextEdit
{
protected:
    bool shouldProcess(QMouseEvent *e)
    {
        return e->modifiers() & Qt::ControlModifier && e->button() & Qt::LeftButton;
    }

    void mousePressEvent(QMouseEvent *e)
    {
        QString href = anchorAt(e->pos());
        if (!href.isEmpty() && shouldProcess(e))
            _clickedAnchor = href;
        else QTextEdit::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent *e)
    {
        if (!_clickedAnchor.isEmpty() && shouldProcess(e))
        {
            QDesktopServices::openUrl(_clickedAnchor);
            _clickedAnchor.clear();
        }
        else QTextEdit::mouseReleaseEvent(e);
    }
private:
    QString _clickedAnchor;
};

//------------------------------------------------------------------------------

MemoWindow::MemoWindow(Catalog *catalog, MemoItem *memoItem) : QWidget(),
    _catalog(catalog), _memoItem(memoItem)
{
    setWindowIcon(QIcon(":/icon/memo_plain_text"));

    _memoEditor = new MemoEditor;
    _memoEditor->setAcceptRichText(false);
    _memoEditor->setWordWrapMode(QTextOption::NoWrap);

    _titleEditor = new QLineEdit;
    _titleEditor->setFont(QFont("Arial", 14));

    auto toolbar = new QToolBar;
    _actionEdit = toolbar->addAction(QIcon(":/toolbar/memo_edit"), tr("Edit"), this, &MemoWindow::beginEditing);
    _actionEdit->setShortcut(QKeySequence(Qt::Key_Return, Qt::Key_Return));
    _actionSave = toolbar->addAction(QIcon(":/toolbar/memo_save"), tr("Save"), this, &MemoWindow::saveEditing);
    _actionSave->setShortcut(QKeySequence::Save);
    _actionCancel = toolbar->addAction(QIcon(":/toolbar/memo_cancel"), tr("Cancel"), this, &MemoWindow::cancelEditing);
    _actionCancel->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_Escape));
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/memo_close"), tr("Close"), [this](){
        parentWidget()->close();
    });

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
    applyTextStyles();

    // Modified flag should be reset after text style was applied
    _memoEditor->document()->setModified(false);
    _titleEditor->setModified(false);
}

void MemoWindow::beginEditing()
{
    toggleEditMode(true);
    _memoEditor->setFocus();
}

void MemoWindow::cancelEditing()
{
    toggleEditMode(false);
    showMemo();
}

bool MemoWindow::saveEditing()
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
        return false;
    }

    setWindowTitle(_memoItem->memo()->title());
    toggleEditMode(false);
    applyTextStyles();

    // Modified flag should be reset after text style was applied
    _memoEditor->document()->setModified(false);
    _titleEditor->setModified(false);

    return true;
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

void MemoWindow::setWordWrap(bool wrap)
{
    _memoEditor->setWordWrapMode(wrap ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
}

void MemoWindow::applyTextStyles()
{
    _memoEditor->setUndoRedoEnabled(false);
    processHyperlinks();
    // Should be applied after hyperlinks to get correct finish style.
    applyHighlighter();
    _memoEditor->setUndoRedoEnabled(true);
}

void MemoWindow::processHyperlinks()
{
    static QList<QRegExp> rex;
    if (rex.isEmpty())
    {
        rex.append(QRegExp("\\bhttp(s?)://[^\\s]+\\b", Qt::CaseInsensitive));
    }
    for (const QRegExp& re : rex)
    {
        QTextCursor cursor = _memoEditor->document()->find(re);
        while (!cursor.isNull())
        {
            QString href = cursor.selectedText();
            qDebug() << href;
            QTextCharFormat f;
            f.setAnchor(true);
            f.setAnchorHref(href);
            f.setForeground(Qt::blue);
            f.setFontUnderline(true);
            // Wanted to draw Ctrl+Click bolded, but when html,
            // tooltip becomes word-wrapped at some rather narrow width and it looks too ugly.
            f.setToolTip(href + tr("\nCtrl + Click to open"));
            cursor.mergeCharFormat(f);

            cursor = _memoEditor->document()->find(re, cursor);
        }
    }
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

bool MemoWindow::isModified() const
{
    return _memoEditor->document()->isModified() || _titleEditor->isModified();
}
