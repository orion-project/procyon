#include "MemoPage.h"

#include "PageWidgets.h"
#include "../editors/MarkdownMemoEditor.h"
#include "../editors/PlainTextMemoEditor.h"
#include "../catalog/Catalog.h"
#include "../catalog/CatalogStore.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriWidgets.h"

#include <QIcon>
#include <QDebug>
#include <QMessageBox>
#include <QToolButton>

namespace {
const int PREVIEW_BUTTON_WIDTH = 100;

namespace MemoOptions {
    const QString font =
#if defined (Q_OS_WIN)
        "fontWin"
#elif defined (Q_OS_LINUX)
        "fontLinux"
#elif defined (Q_OS_MAC)
        "fontMacos"
#else
        "font"
#endif
    ;
    const QString wordWrap = "wordWrap";
    const QString spellcheck = "spellcheck";
    const QString highlighter = "highlighter";
};

void updateOption(MemoItem* memo, const QString& name, const QVariant& value)
{
    QString res = CatalogStore::memoManager()->updateOption(memo->id(), name, value);
    if (!res.isEmpty())
        Ori::Dlg::error(QString("Unable to store memo option in catalog.\n\n%1").arg(res));
}
}


MemoPage::MemoPage(Catalog *catalog, MemoItem *memoItem) : QWidget(),
    _catalog(catalog), _memoItem(memoItem)
{
    auto memoType = _memoItem->type();

    setWindowIcon(memoType->icon());

    if (memoType == markdownMemoType())
        _memoEditor = new MarkdownMemoEditor(_memoItem);
    else
        _memoEditor = new PlainTextMemoEditor(_memoItem);
    connect(_memoEditor, &MemoEditor::onModified, this, &MemoPage::onModified);

    _titleEditor = PageWidgets::makeTitleEditor();
    connect(_titleEditor, &QLineEdit::textEdited, [this]{ emit onModified(true); });

    _toolbar = new QToolBar;
    _toolbar->setObjectName("memo_toolbar");
    _toolbar->setContentsMargins(0, 0, 0, 0);
    _toolbar->setIconSize(QSize(24, 24));

    _actionEdit = _toolbar->addAction(QIcon(":/toolbar/memo_edit"), tr("Edit"), this, &MemoPage::beginEdit);
    _actionSave = _toolbar->addAction(QIcon(":/toolbar/memo_save"), tr("Save"), this, &MemoPage::saveEdit);
    _actionCancel = _toolbar->addAction(QIcon(":/toolbar/memo_cancel"), tr("Cancel"), this, &MemoPage::cancelEdit);
    _actionEdit->setShortcut(QKeySequence(Qt::Key_Return, Qt::Key_Return));
    _actionSave->setShortcut(QKeySequence::Save);
    _actionCancel->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_Escape));
    _toolbar->addSeparator();
    _toolbar->addAction(QIcon(":/toolbar/memo_close"), tr("Close"), [this](){
        if (canClose()) deleteLater();
    });

    auto toolPanel = PageWidgets::makeHeaderPanel({_titleEditor, _toolbar});

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

    if (_memoItem->type() == markdownMemoType())
    {
        if (on)
        {
            _actionPreview = new QAction(tr("Edit"));
            _actionPreview->setShortcut(Qt::Key_F5);
            _actionPreview->setToolTip(tr("Switch between Preview and Edit mode"));
            connect(_actionPreview, &QAction::triggered, this, &MemoPage::togglePreviewMode);

            _previewButton = new QToolButton;
            _previewButton->setObjectName("button_preview");
            _previewButton->setDefaultAction(_actionPreview);
            _previewButton->setFixedWidth(PREVIEW_BUTTON_WIDTH);

            auto firstAction = _toolbar->actions().first();
            _actionPreviewButton = _toolbar->insertWidget(firstAction, _previewButton);
            _separatorPreview = _toolbar->insertSeparator(firstAction);
        }
        else if (_actionPreview)
        {
            delete _actionPreview;
            delete _previewButton;
            delete _actionPreviewButton;
            delete _separatorPreview;
        }
    }

    _titleEditor->setReadOnly(!on);
    // Force updating editor's style sheet, seems it doesn't note changing of readOnly or a custom property
    _titleEditor->setStyleSheet(QString("QLineEdit { background: %1 }").arg(on ? "white" : "transparent"));
}

QFont MemoPage::memoFont() const
{
    // TODO: get font from memo
    return AppSettings::instance().memoFont;
}

void MemoPage::setMemoFont(const QFont& font)
{
    _memoEditor->setFont(font);
    // TODO: choose default memo from in different place
    //AppSettings::instance().memoFont = font;
    updateOption(_memoItem, MemoOptions::font, font.toString());
}

bool MemoPage::wordWrap() const
{
    // TODO: get option from memo
    return AppSettings::instance().memoWordWrap;
}

void MemoPage::setWordWrap(bool wrap)
{
    _memoEditor->setWordWrap(wrap);
    // TODO: don't store this flag in settings (?)
    //AppSettings::instance().memoWordWrap = wrap;
    updateOption(_memoItem, MemoOptions::wordWrap, wrap);
}

bool MemoPage::isModified() const
{
    return _memoEditor->isModified() || _titleEditor->isModified();
}

void MemoPage::setSpellcheckLang(const QString &lang)
{
    _memoEditor->setSpellcheckLang(lang);
    updateOption(_memoItem, MemoOptions::spellcheck, lang);
}

QString MemoPage::spellcheckLang() const
{
    return _memoEditor->spellcheckLang();
}

void MemoPage::setHighlighter(const QString& name)
{
    auto editor = dynamic_cast<PlainTextMemoEditor*>(_memoEditor);
    if (editor)
    {
        editor->setHighlighterName(name);
        updateOption(_memoItem, MemoOptions::highlighter, name);
    }
}

QString MemoPage::highlighter() const
{
    auto editor = dynamic_cast<PlainTextMemoEditor*>(_memoEditor);
    return editor ? editor->highlighterName() : QString();
}

void MemoPage::togglePreviewMode()
{
    auto editor = qobject_cast<MarkdownMemoEditor*>(_memoEditor);
    if (!editor) return;

    bool isPreview = editor->isPreviewMode();
    _actionPreview->setText(isPreview ? tr("Edit") : tr("Preview"));
    editor->togglePreviewMode(!isPreview);
}

void MemoPage::loadSettings()
{
    auto options = CatalogStore::memoManager()->selectOptions(_memoItem->id());

    auto memoFont = AppSettings::instance().memoFont;
    if (options.contains(MemoOptions::font))
        memoFont.fromString(options[MemoOptions::font].toString());
    _memoEditor->setFont(memoFont);

    _memoEditor->setWordWrap(options.contains(MemoOptions::wordWrap)
        ? options[MemoOptions::wordWrap].toBool() : AppSettings::instance().memoWordWrap);

    if (options.contains(MemoOptions::spellcheck))
        _memoEditor->setSpellcheckLang(options[MemoOptions::spellcheck].toString());

    if (options.contains(MemoOptions::highlighter))
    {
        auto editor = dynamic_cast<PlainTextMemoEditor*>(_memoEditor);
        if (editor) editor->setHighlighterName(options[MemoOptions::highlighter].toString());
    }
}
