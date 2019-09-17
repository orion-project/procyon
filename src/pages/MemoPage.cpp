#include "MemoPage.h"

#include "PageWidgets.h"
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
    setWindowIcon(QIcon(":/icon/memo_plain_text"));

    _memoEditor = new PlainTextMemoEditor(_memoItem);
    connect(_memoEditor, &MemoEditor::onModified, this, &MemoPage::onModified);

    _titleEditor = PageWidgets::makeTitleEditor();

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
    _memoEditor->showMemo(_memoItem);
    _titleEditor->setText(_memoItem->title());
    setWindowTitle(_memoItem->title());

    _memoEditor->setUnmodified();
    _titleEditor->setModified(false);
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
    if (!saveEditing()) return false;

    return true;
}

void MemoPage::beginEditing()
{
    toggleEditMode(true);
    _memoEditor->setFocus();
    emit onReadOnly(false);
}

void MemoPage::cancelEditing()
{
    toggleEditMode(false);
    showMemo();
    emit onReadOnly(true);
}

bool MemoPage::saveEditing()
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

    setWindowTitle(_memoItem->title());
    toggleEditMode(false);

    _memoEditor->setUnmodified();
    _titleEditor->setModified(false);

    emit onReadOnly(true);
    return true;
}

void MemoPage::toggleEditMode(bool on)
{
    _actionSave->setVisible(on);
    _actionCancel->setVisible(on);
    _actionEdit->setVisible(!on);

    _memoEditor->setReadOnly(!on);

    _titleEditor->setReadOnly(!on);
    // Force updating editor's style sheet, seems it doesn't note changing of readOnly or a custom property
    _titleEditor->setStyleSheet(QString("QLineEdit { background: %1 }").arg(on ? "white" : "transparent"));

    _memoEditor->toggleSpellcheck(on);
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

bool MemoPage::isReadOnly() const
{
    return _memoEditor->isReadOnly();
}

void MemoPage::setSpellcheckLang(const QString &lang)
{
    _memoEditor->setSpellcheckLang(lang);
}

QString MemoPage::spellcheckLang() const
{
    return _memoEditor->spellcheckLang();
}
