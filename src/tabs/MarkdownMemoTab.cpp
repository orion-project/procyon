#include "MarkdownMemoTab.h"

#include "AppSettings.h"
#include "TextEditHelpers.h"
#include "core/Enot.h"
#include "core/MemoStore.h"
#include "markdown/MarkdownHelper.h"
#include "tabs/TabHelpers.h"
#include "widgets/MemoPropsPanel.h"
#include "widgets/MemoTextBrowser.h"
#include "widgets/MemoTextEdit.h"

#include "tools/OriSpellcheck.h"

#include <QLineEdit>
#include <QMenu>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QStackedLayout>

typedef MarkdownMemoTab Self;

MarkdownMemoTab::MarkdownMemoTab(Enot* enot, Memo* memo) : MemoTab(enot, memo)
{
    _textView = new MemoTextBrowser;
    _textView->document()->setDefaultStyleSheet(AppSettings::instance().markdownCss());
    _textView->document()->setDocumentMargin(10);

    _spellcheck = new Ori::Spellcheck(_textEditor);

    _titleEditor = TabHelpers::makeTitleEditor();
    connect(_titleEditor, &QLineEdit::textEdited, [this]{ emit onModified(true); });

    auto toolbar = TabHelpers::makeHeaderToolBar();

    auto toolMenu = new QMenu(this);
    auto actionFont = toolMenu->addAction(tr("Choose Font..."), this, &Self::chooseFont);
    auto actionWordWrap = toolMenu->addAction(tr("Word Wrap..."), this, &Self::toggleWordWrap);
    actionWordWrap->setCheckable(true);
    auto actionAddProp = toolMenu->addAction(tr("Add Property..."), this, [this]{ _propsPanel->addPropViaDlg(); });
    toolMenu->addAction(tr("Export to PDF..."), this, [this]{ TextEditHelpers::exportToPdfDlg(_textEditor); });
    _spellcheckMenu = toolMenu->addMenu(tr("Spellcheck"));
    connect(_spellcheckMenu, &QMenu::aboutToShow, this, &Self::showSelectedSpellcheckLang);
    Ori::Spellcheck::fillMenu(_spellcheckMenu, [this](const QString& lang){
        if (lang != _spellcheckLang)
            _spellcheckLang = lang;
        else _spellcheckLang.clear();
        _spellcheck->setLang(_spellcheckLang);
        _enot->updateMemoOption(_memo->id(), MemoOptions::SPELLCHECK, _spellcheckLang);
    });
    connect(toolMenu, &QMenu::aboutToShow, this, [this, actionWordWrap, actionAddProp, actionFont]{
        bool isEditMode = !isReadOnly();
        actionFont->setEnabled(isEditMode);
        actionWordWrap->setChecked(_textEditor->wordWrap());
        actionWordWrap->setEnabled(isEditMode);
        actionAddProp->setEnabled(isEditMode);
        _spellcheckMenu->setEnabled(isEditMode);
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

    _tabs = new QStackedLayout;

    Ori::Layouts::LayoutV({toolPanel, _propsPanel, _tabs}).setMargin(0).setSpacing(0).useFor(this);

    _propsPanel->setValues(memo->props());

    _tabs->addWidget(_textView);
    _tabs->setCurrentWidget(_textView);

    showMemo();
    toggleEditMode(false);

    QTimer::singleShot(0, this, [this](){
        TextEditHelpers::adjustDocumentWidth(_textEditor);

        if (_memo->title().isEmpty())
            _titleEditor->setFocus();
    });
}

void MarkdownMemoTab::showMemo()
{
    _titleEditor->setText(_memo->title());
    _titleEditor->setModified(false);

    _textView->setHtml(MarkdownHelper::markdownToHtml(_memo->data()));
    _textEditor->setModified(false);

    setWindowTitle(_memo->title());
}

bool MarkdownMemoTab::isModified() const
{
    return _textEditor->isModified() || _titleEditor->isModified();
}

bool MarkdownMemoTab::isReadOnly() const
{
    return _textEditor->isReadOnly();
}

void MarkdownMemoTab::toggleEditMode(bool on)
{
    TabHelpers::setTitleEditorReadOnly(_titleEditor, !on);
    _tabs->setCurrentIndex(on ? 0 : 1);
    _propsPanel->setReadOnly(!on);

    _actionSave->setVisible(on);
    _actionCancel->setVisible(on);
    _actionEdit->setVisible(!on);

    _spellcheck->setLang(on ? _spellcheckLang : QString());
}

void MarkdownMemoTab::beginEdit()
{
    if (!_textEditor)
    {
        _textEditor = new MemoTextEdit;
        connect(_textEditor, &MemoTextEdit::undoAvailable, this, &Self::onModified);

        auto options = Store::memos()->selectOptions(_memo->id());

        auto memoFont = AppSettings::instance().memoFont;
        if (options.contains(MemoOptions::FONT))
            memoFont.fromString(options[MemoOptions::FONT].toString());
        _textEditor->setFont(memoFont);

        _textEditor->setWordWrap(options.contains(MemoOptions::WORD_WRAP)
            ? options[MemoOptions::WORD_WRAP].toBool() : AppSettings::instance().memoWordWrap);

        if (options.contains(MemoOptions::SPELLCHECK))
            _spellcheckLang = options[MemoOptions::SPELLCHECK].toString();

        _tabs->addWidget(_textEditor);
    }

    _textEditor->setPlainText(_memo->data());
    _textEditor->setModified(false);

    toggleEditMode(true);

    if (_memo->data().isEmpty())
    {
        _titleEditor->setFocus();
        _titleEditor->selectAll();
    }
    else
        _textEditor->setFocus();

    emit onReadOnly(false);
}

void MarkdownMemoTab::cancelEdit()
{
    toggleEditMode(false);

    showMemo();

    emit onReadOnly(true);
}

bool MarkdownMemoTab::saveEdit()
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

    emit onReadOnly(true);
    return true;
}

bool MarkdownMemoTab::canClose()
{
    return !isModified() || TextEditHelpers::canClose(_titleEditor, [this]{ return saveEdit(); });
}

void MarkdownMemoTab::chooseFont()
{
    if (TextEditHelpers::chooseFontDlg(_textEditor))
        _enot->updateMemoOption(_memo->id(), MemoOptions::FONT, _textEditor->font().toString());
}

void MarkdownMemoTab::toggleWordWrap()
{
    _textEditor->setWordWrap(!_textEditor->wordWrap());
    _enot->updateMemoOption(_memo->id(), MemoOptions::WORD_WRAP, _textEditor->wordWrap());
}

void MarkdownMemoTab::showSelectedSpellcheckLang()
{
    auto actions = _spellcheckMenu->actions();
    for (auto action : std::as_const(actions))
        action->setChecked(action->data().toString() == _spellcheckLang);
}
