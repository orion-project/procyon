#include "MemoPage.h"

#include "PageWidgets.h"
#include "../editors/MarkdownMemoEditor.h"
#include "../editors/PlainTextMemoEditor.h"
#include "../catalog/Catalog.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriWidgets.h"

#include <QIcon>
#include <QDebug>
#include <QMessageBox>

MemoPage::MemoPage(Catalog *catalog, MemoItem *memoItem) : QWidget(),
    _catalog(catalog), _memoItem(memoItem)
{
    setWindowIcon(_memoItem->type()->icon());

    if (_memoItem->type() == markdownMemoType())
        _memoEditor = new MarkdownMemoEditor(_memoItem);
    else
        _memoEditor = new PlainTextMemoEditor(_memoItem);
    connect(_memoEditor, &MemoEditor::onModified, this, &MemoPage::onModified);

    _titleEditor = PageWidgets::makeTitleEditor();
    connect(_titleEditor, &QLineEdit::textEdited, [this]{ emit onModified(true); });

    auto toolbar = new QToolBar;
    toolbar->setObjectName("memo_toolbar");
    toolbar->setContentsMargins(0, 0, 0, 0);
    toolbar->setIconSize(QSize(24, 24));
    _actionEdit = toolbar->addAction(QIcon(":/toolbar/memo_edit"), tr("Edit"), this, &MemoPage::beginEdit);
    _actionSave = toolbar->addAction(QIcon(":/toolbar/memo_save"), tr("Save"), this, &MemoPage::saveEdit);
    _actionCancel = toolbar->addAction(QIcon(":/toolbar/memo_cancel"), tr("Cancel"), this, &MemoPage::cancelEdit);
    _actionEdit->setShortcut(QKeySequence(Qt::Key_Return, Qt::Key_Return));
    _actionSave->setShortcut(QKeySequence::Save);
    _actionCancel->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_Escape));
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/memo_close"), tr("Close"), [this](){
        if (canClose()) deleteLater();
    });

    auto toolPanel = PageWidgets::makeHeaderPanel({_titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, _memoEditor}).setMargin(0).setSpacing(0).useFor(this);

    showMemo();
    toggleEditMode(false);
    _memoEditor->setFocus();
}

MemoPage::~MemoPage()
{
}

void MemoPage::showMemo()
{
    _memoEditor->showMemo();

    _titleEditor->setText(_memoItem->title());
    _titleEditor->setModified(false);

    setWindowTitle(_memoItem->title());
}

bool MemoPage::canClose()
{
    if (!isModified()) return true;

    int res = Ori::Dlg::yesNoCancel(tr("<b>%1</b><br/><br/>"
                                       "This memo has been changed. "
                                       "Save changes before closing?")
                                    .arg(windowTitle()));
    if (res == QMessageBox::Cancel) return false;
    if (res == QMessageBox::No) return true;
    if (!saveEdit()) return false;

    return true;
}

void MemoPage::beginEdit()
{
    toggleEditMode(true);
    _memoEditor->beginEdit();
    emit onReadOnly(false);
}

void MemoPage::cancelEdit()
{
    toggleEditMode(false);
    _memoEditor->endEdit();
    showMemo();
    emit onReadOnly(true);
}

bool MemoPage::saveEdit()
{
    MemoUpdateParam update;
    update.title = _titleEditor->text().trimmed();
    update.data = _memoEditor->data();

    auto res = _catalog->updateMemo(_memoItem, update);
    if (!res.isEmpty())
    {
        Ori::Dlg::error(res);
        return false;
    }

    _memoEditor->saveEdit();
    _titleEditor->setModified(false);
    setWindowTitle(_memoItem->title());
    toggleEditMode(false);
    emit onReadOnly(true);
    return true;
}

void MemoPage::toggleEditMode(bool on)
{
    _isEditMode = on;

    _actionSave->setVisible(on);
    _actionCancel->setVisible(on);
    _actionEdit->setVisible(!on);

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
    _memoEditor->setWordWrap(wrap);
}

bool MemoPage::isModified() const
{
    return _memoEditor->isModified() || _titleEditor->isModified();
}

void MemoPage::setSpellcheckLang(const QString &lang)
{
    _memoEditor->setSpellcheckLang(lang);
}

QString MemoPage::spellcheckLang() const
{
    return _memoEditor->spellcheckLang();
}
