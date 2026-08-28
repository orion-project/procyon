#include "PlainTextMemoTab.h"

#include "AppSettings.h"
#include "TextEditHelpers.h"
#include "core/Enot.h"
#include "core/MemoStore.h"
#include "tabs/TabHelpers.h"
#include "widgets/MemoPropsPanel.h"
#include "widgets/MemoTextEdit.h"

#include <QLineEdit>
#include <QMenu>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

typedef PlainTextMemoTab Self;

PlainTextMemoTab::PlainTextMemoTab(Enot* enot, Memo* memo) : MemoTab(enot, memo)
{
    _textEditor = new MemoTextEdit;
    connect(_textEditor, &MemoTextEdit::undoAvailable, this, &PlainTextMemoTab::onModified);

    _titleEditor = TabHelpers::makeTitleEditor();
    connect(_titleEditor, &QLineEdit::textEdited, [this]{ emit onModified(true); });

    auto toolbar = TabHelpers::makeHeaderToolBar();

    auto toolMenu = new QMenu(this);
    toolMenu->addAction(tr("Choose Font..."), this, &Self::chooseFont);
    auto actionWordWrap = toolMenu->addAction(tr("Word Wrap..."), this, &Self::toggleWordWrap);
    actionWordWrap->setCheckable(true);
    auto actionAddProp = toolMenu->addAction(tr("Add Property..."), this, [this]{ _propsPanel->addPropViaDlg(); });
    toolMenu->addAction(tr("Export to PDF..."), this, [this]{ TextEditHelpers::exportToPdfDlg(_textEditor); });
    connect(toolMenu, &QMenu::aboutToShow, this, [this, actionWordWrap, actionAddProp]{
        actionWordWrap->setChecked(_textEditor->wordWrap());
        actionAddProp->setEnabled(!isReadOnly());
    });

    _actionEdit = toolbar->addAction(QIcon(":/toolbar/edit"), tr("Edit"), this, &Self::beginEdit);
    _actionSave = toolbar->addAction(QIcon(":/toolbar/apply"), tr("Save"), this, &Self::saveEdit);
    _actionCancel = toolbar->addAction(QIcon(":/toolbar/cancel"), tr("Cancel"), this, &Self::cancelEdit);
    _actionEdit->setShortcut(QKeySequence(Qt::Key_Return, Qt::Key_Return));
    _actionSave->setShortcut(QKeySequence::Save);
    _actionCancel->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_Escape));
    toolbar->addSeparator();
    toolbar->addWidget(TabHelpers::makeMenuButton(toolMenu, tr("Options")));
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/toolbar/close"), tr("Close Tab"), [this](){
        if (canClose()) deleteLater();
    });

    auto toolPanel = TabHelpers::makeHeaderPanel({_titleEditor, toolbar});

    _propsPanel = new MemoPropsPanel(enot);
    _propsPanel->setVisible(false);

    Ori::Layouts::LayoutV({toolPanel, _propsPanel, _textEditor}).setMargin(0).setSpacing(0).useFor(this);

    _propsPanel->setValues(memo->props());

    showMemo();
    toggleEditMode(false);

    QTimer::singleShot(0, this, [this](){
        TextEditHelpers::adjustDocumentWidth(_textEditor);

        if (_memo->title().isEmpty())
            _titleEditor->setFocus();
    });
}

void PlainTextMemoTab::showMemo()
{
    _titleEditor->setText(_memo->title());
    _titleEditor->setModified(false);

    _textEditor->setPlainText(_memo->data());
    _textEditor->setModified(false);

    setWindowTitle(_memo->title());
}

bool PlainTextMemoTab::isModified() const
{
    return _textEditor->isModified() || _titleEditor->isModified();
}

bool PlainTextMemoTab::isReadOnly() const
{
    return _textEditor->isReadOnly();
}

void PlainTextMemoTab::toggleEditMode(bool on)
{
    TabHelpers::setTitleEditorReadOnly(_titleEditor, !on);
    _textEditor->setReadOnly(!on);
    _propsPanel->setReadOnly(!on);

    _actionSave->setVisible(on);
    _actionCancel->setVisible(on);
    _actionEdit->setVisible(!on);
}

void PlainTextMemoTab::beginEdit()
{
    toggleEditMode(true);

    if (_memo->data().isEmpty())
    {
        _titleEditor->setFocus();
        _titleEditor->selectAll();
    }
    else
        _textEditor->setFocus();

    // TODO toggleSpellcheck(true);

    emit onReadOnly(false);
}

void PlainTextMemoTab::cancelEdit()
{
    toggleEditMode(false);

    showMemo();

    // TODO: toggleSpellcheck(false);

    emit onReadOnly(true);
}

bool PlainTextMemoTab::saveEdit()
{
    _propsPanel->apply();

    MemoUpdateParam update;
    QString newTitle = _titleEditor->text().trimmed();
    if (newTitle != _memo->title())
        update.title = newTitle;
    if (_textEditor->isModified())
        update.data = _textEditor->toPlainText();
    if (_propsPanel->hasValues())
        update.props = _propsPanel->values();

    auto ok = _enot->updateMemo(_memo, update);
    if (!ok) return false;

    _titleEditor->setModified(false);
    _textEditor->setModified(false);
    setWindowTitle(_memo->title());
    toggleEditMode(false);

    // TODO: toggleSpellcheck(false);

    emit onReadOnly(true);
    return true;
}

void PlainTextMemoTab::loadSettings()
{
    auto options = Store::memos()->selectOptions(_memo->id());

    auto memoFont = AppSettings::instance().memoFont;
    if (options.contains(MemoOptions::FONT))
        memoFont.fromString(options[MemoOptions::FONT].toString());
    _textEditor->setFont(memoFont);

    _textEditor->setWordWrap(options.contains(MemoOptions::WORD_WRAP)
        ? options[MemoOptions::WORD_WRAP].toBool() : AppSettings::instance().memoWordWrap);

    // if (options.contains(MemoOptions::SPELLCHECK))
    //     _memoEditor->setSpellcheckLang(options[MemoOptions::SPELLCHECK].toString());

    // if (options.contains(MemoOptions::HIGHLIGHTER))
    // {
    //     auto editor = dynamic_cast<TextMemoEditor*>(_memoEditor);
    //     if (editor) editor->setHighlighterName(options[MemoOptions::HIGHLIGHTER].toString());
    // }
}

bool PlainTextMemoTab::canClose()
{
    return isModified() && TextEditHelpers::canClose(_titleEditor, [this]{ return saveEdit(); });
}

void PlainTextMemoTab::chooseFont()
{
    if (TextEditHelpers::chooseFontDlg(_textEditor))
        _enot->updateMemoOption(_memo->id(), MemoOptions::FONT, _textEditor->font().toString());
}

void PlainTextMemoTab::toggleWordWrap()
{
    _textEditor->setWordWrap(!_textEditor->wordWrap());
    _enot->updateMemoOption(_memo->id(), MemoOptions::WORD_WRAP, _textEditor->wordWrap());
}
