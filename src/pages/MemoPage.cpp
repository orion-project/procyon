#include "MemoPage.h"

#include "../catalog/Catalog.h"
#include "../catalog/Memo.h"
#include "../highlighter/PythonSyntaxHighlighter.h"
#include "../highlighter/ShellMemoSyntaxHighlighter.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "helpers/OriWidgets.h"

#include <QIcon>
#include <QDebug>
#include <QFrame>
#include <QLineEdit>
#include <QMessageBox>
#include <QStyle>
#include <QTextEdit>
#include <QTimer>
#include <QToolBar>
#include <QDesktopServices>

using namespace Ori::Layouts;

//------------------------------------------------------------------------------
//                               MemoEditor
//------------------------------------------------------------------------------

bool MemoEditor::shouldProcess(QMouseEvent *e)
{
    return e->modifiers() & Qt::ControlModifier && e->button() & Qt::LeftButton;
}

void MemoEditor::mousePressEvent(QMouseEvent *e)
{
    QString href = anchorAt(e->pos());
    if (!href.isEmpty() && shouldProcess(e))
        _clickedAnchor = href;
    else QTextEdit::mousePressEvent(e);
}

void MemoEditor::mouseReleaseEvent(QMouseEvent *e)
{
    if (!_clickedAnchor.isEmpty() && shouldProcess(e))
    {
        QDesktopServices::openUrl(_clickedAnchor);
        _clickedAnchor.clear();
    }
    else QTextEdit::mouseReleaseEvent(e);
}

//------------------------------------------------------------------------------
//                                 MemoPage
//------------------------------------------------------------------------------

MemoPage::MemoPage(Catalog *catalog, MemoItem *memoItem) : QWidget(),
    _catalog(catalog), _memoItem(memoItem)
{
    setWindowIcon(QIcon(":/icon/memo_plain_text"));

    _memoEditor = new MemoEditor;
    _memoEditor->setAcceptRichText(false);
    _memoEditor->setWordWrapMode(QTextOption::NoWrap);
    _memoEditor->setProperty("role", "memo_editor");

    _titleEditor = new QLineEdit;
    _titleEditor->setObjectName("memo_title_editor");
    _titleEditor->setProperty("role", "memo_title");

    auto toolbar = new QToolBar;
    toolbar->setObjectName("memo_toolbar");
    toolbar->setContentsMargins(0, 0, 0, 0);
    toolbar->setIconSize(QSize(24, 24));
    _actionEdit = toolbar->addAction(QIcon(":/toolbar/memo_edit"), tr("Edit"), this, &MemoPage::beginEditing);
    _actionSave = toolbar->addAction(QIcon(":/toolbar/memo_save"), tr("Save"), this, &MemoPage::saveEditing);
    _actionCancel = toolbar->addAction(QIcon(":/toolbar/memo_cancel"), tr("Cancel"), this, &MemoPage::cancelEditing);
    _actionEdit->setShortcut(QKeySequence(Qt::Key_Return, Qt::Key_Return));
    _actionSave->setShortcut(QKeySequence::Save);
    _actionCancel->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_Escape));
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/memo_close"), tr("Close"), [this](){
        if (canClose()) deleteLater();
    });

    auto toolPanel = new QFrame;
    toolPanel->setObjectName("memo_header_panel");
    LayoutH({_titleEditor, toolbar}).setMargin(0).useFor(toolPanel);

    LayoutV({toolPanel, _memoEditor}).setMargin(0).setSpacing(0).useFor(this);

    showMemo();
    toggleEditMode(false);
    _memoEditor->setFocus();

    QTimer::singleShot(0, [this](){
        auto sb = 1.5 * style()->pixelMetric(QStyle::PM_ScrollBarExtent);
        _memoEditor->document()->setTextWidth(_memoEditor->width() - sb);
    });
}

MemoPage::~MemoPage()
{
}

void MemoPage::showMemo()
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

bool MemoPage::canClose()
{
    if (!isModified()) return true;

    int res = Ori::Dlg::yesNoCancel(tr("<b>%1</b><br><br>"
                                       "This memo has been changed. "
                                       "Save changes before closing?")
                                    .arg(windowTitle()));
    if (res == QMessageBox::Cancel) return false;
    if (res == QMessageBox::No) return true;
    if (!saveEditing()) return false;

    return true;
}

void MemoPage::beginEditing()
{
    toggleEditMode(true);
    _memoEditor->setFocus();
}

void MemoPage::cancelEditing()
{
    toggleEditMode(false);
    showMemo();
}

bool MemoPage::saveEditing()
{
    auto memo = _memoItem->type()->makeMemo();
    // TODO preserve additional non editable data - dates, etc.
    memo->setId(_memoItem->memo()->id());
    memo->setTitle(_titleEditor->text().trimmed());
    memo->setData(_memoEditor->toPlainText());

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

void MemoPage::toggleEditMode(bool on)
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
    // Force updating editor's style sheet, seems it doesn't note changing of readOnly or a custom property
    _titleEditor->setStyleSheet(QString("QLineEdit { background: %1 }").arg(on ? "white" : "transparent"));
}

void MemoPage::setMemoFont(const QFont& font)
{
    _memoEditor->setFont(font);
}

void MemoPage::setWordWrap(bool wrap)
{
    _memoEditor->setWordWrapMode(wrap ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
}

void MemoPage::applyTextStyles()
{
    _memoEditor->setUndoRedoEnabled(false);
    processHyperlinks();
    // Should be applied after hyperlinks to get correct finish style.
    applyHighlighter();
    _memoEditor->setUndoRedoEnabled(true);
}

void MemoPage::processHyperlinks()
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

void MemoPage::applyHighlighter()
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

bool MemoPage::isModified() const
{
    return _memoEditor->document()->isModified() || _titleEditor->isModified();
}
