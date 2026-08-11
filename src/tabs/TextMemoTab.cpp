#include "TextMemoTab.h"

#include "TabHelpers.h"
#include "core/Enot.h"
#include "core/MemoStore.h"
#include "core/MemoType.h"
#include "editors/MarkdownMemoEditor.h"
#include "editors/MemoEditor.h"

#include "helpers/OriDialogs.h"
#include "widgets/OriFlowLayout.h"

#include <QMessageBox>
#include <QToolBar>
#include <QToolButton>
#include <QLabel>

namespace {
const int PREVIEW_BUTTON_WIDTH = 100;

namespace MemoOptions {
const QString FONT =
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
const QString WORD_WRAP = "wordWrap";
const QString SPELLCHECK = "spellcheck";
const QString HIGHLIGHTER = "highlighter";
};

void updateOption(Memo* memo, const QString& name, const QVariant& value)
{
    QString res = Store::memos()->updateOption(memo->id(), name, value);
    if (!res.isEmpty())
        Ori::Dlg::error(QString("Unable to store memo option in database.\n\n%1").arg(res));
}
}

//------------------------------------------------------------------------------
//                                MemoPropsPanel
//------------------------------------------------------------------------------

class MemoPropsPanel : public QFrame
{
public:
    MemoPropsPanel() : QFrame()
    {
        setObjectName("props_panel");

        _layout = new Ori::Widgets::FlowLayout(this, 0, 0, 5);
    }

    void addProp(const QString& name, const QString& value)
    {
        auto labelName = new QLabel(name);
        labelName->setProperty("role", "prop_name");

        auto labelValue = new QLabel(value);
        labelValue->setProperty("role", "prop_value");

        auto widget = Ori::Layouts::LayoutH({labelName, labelValue}).setMargin(0).setSpacing(0).makeWidget();

        _valueViews.append(ValueView{
            .name = name,
            .value = value,
            .contentWidget = widget,
            .readonlyLabel = labelValue,
        });

        _layout->addWidget(widget);
    }

    void setReadOnly(bool on)
    {
        if (on) switchToEditable();
        else switchToReadonly();
    }

private:
    QLayout *_layout;
    QMenu *_valueMenu;

    struct ValueView
    {
        QString name;
        QString value;
        QWidget *contentWidget;
        QLabel *readonlyLabel;
        QLabel *editableLabel = nullptr;
    };

    QList<ValueView> _valueViews;

    void switchToEditable()
    {
        for (ValueView& view : _valueViews)
        {
            if (!view.editableLabel)
            {
                view.editableLabel = new QLabel;
                view.editableLabel->setProperty("role", "prop_editor");
                view.editableLabel->setCursor(Qt::PointingHandCursor);
                view.contentWidget->layout()->addWidget(view.editableLabel);
            }

            view.editableLabel->setText(view.value);
            view.editableLabel->setVisible(true);
            view.readonlyLabel->setVisible(false);
        }
    }

    void switchToReadonly()
    {
        for (ValueView& view : _valueViews)
        {
            if (view.editableLabel)
                view.editableLabel->setVisible(false);
            view.readonlyLabel->setText(view.value);
            view.readonlyLabel->setVisible(true);
        }
    }
};

//------------------------------------------------------------------------------
//                                TextMemoTab
//------------------------------------------------------------------------------


TextMemoTab::TextMemoTab(Enot* enot, Memo* memo) : MemoTab(enot, memo)
{
    if (_memo->type() == MemoType::markdown())
        _memoEditor = new MarkdownMemoEditor(_memo);
    else
        _memoEditor = new TextMemoEditor(_memo);
    connect(_memoEditor, &MemoEditor::onModified, this, &TextMemoTab::onModified);

    _titleEditor = TabHelpers::makeTitleEditor();
    connect(_titleEditor, &QLineEdit::textEdited, [this]{ emit onModified(true); });

    _toolbar = TabHelpers::makeHeaderToolBar();

    _actionEdit = _toolbar->addAction(QIcon(":/toolbar/edit"), tr("Edit"), this, &TextMemoTab::beginEdit);
    _actionSave = _toolbar->addAction(QIcon(":/toolbar/apply"), tr("Save"), this, &TextMemoTab::saveEdit);
    _actionCancel = _toolbar->addAction(QIcon(":/toolbar/cancel"), tr("Cancel"), this, &TextMemoTab::cancelEdit);
    _actionEdit->setShortcut(QKeySequence(Qt::Key_Return, Qt::Key_Return));
    _actionSave->setShortcut(QKeySequence::Save);
    _actionCancel->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_Escape));
    _toolbar->addSeparator();
    _toolbar->addAction(QIcon(":/toolbar/close"), tr("Close Tab"), [this](){
        if (canClose()) deleteLater();
    });

    auto toolPanel = TabHelpers::makeHeaderPanel({_titleEditor, _toolbar});

    _propsPanel = new MemoPropsPanel();
    _propsPanel->addProp("Category:", "CheckList");
    _propsPanel->addProp("Severity:", "Task");

    Ori::Layouts::LayoutV({toolPanel, _propsPanel, _memoEditor}).setMargin(0).setSpacing(0).useFor(this);

    showMemo();
    toggleEditMode(false);
    _memoEditor->setFocus();
}

QFont TextMemoTab::memoFont() const
{
    return _memoEditor->font();
}

void TextMemoTab::setMemoFont(const QFont& font)
{
    _memoEditor->setFont(font);
    updateOption(_memo, MemoOptions::FONT, font.toString());
}

bool TextMemoTab::wordWrap() const
{
    return _memoEditor->wordWrap();
}

void TextMemoTab::setWordWrap(bool wrap)
{
    _memoEditor->setWordWrap(wrap);
    updateOption(_memo, MemoOptions::WORD_WRAP, wrap);
}

void TextMemoTab::setSpellcheckLang(const QString &lang)
{
    _memoEditor->setSpellcheckLang(lang);
    updateOption(_memo, MemoOptions::SPELLCHECK, lang);
}

QString TextMemoTab::spellcheckLang() const
{
    return _memoEditor->spellcheckLang();
}

void TextMemoTab::setHighlighter(const QString& name)
{
    auto editor = dynamic_cast<TextMemoEditor*>(_memoEditor);
    if (editor && editor->highlighterName() != name)
    {
        bool modified = editor->isModified();
        editor->setHighlighterName(name);
        updateOption(_memo, MemoOptions::HIGHLIGHTER, name);
        editor->setModified(modified);
    }
}

QString TextMemoTab::highlighter() const
{
    auto editor = dynamic_cast<TextMemoEditor*>(_memoEditor);
    return editor ? editor->highlighterName() : QString();
}

void TextMemoTab::exportToPdf()
{
    auto editor = dynamic_cast<TextMemoEditor*>(_memoEditor);
    if (!editor) return;

    QString fileName = Ori::Dlg::getSaveFileName(
        tr("Export memo as PDF"), tr("PDF documents (*.pdf);;All files (*.*)"), "pdf");
    if (fileName.isEmpty()) return;

    editor->exportToPdf(fileName);
}

void TextMemoTab::loadSettings()
{
    auto options = Store::memos()->selectOptions(_memo->id());

    auto memoFont = AppSettings::instance().memoFont;
    if (options.contains(MemoOptions::FONT))
        memoFont.fromString(options[MemoOptions::FONT].toString());
    _memoEditor->setFont(memoFont);

    _memoEditor->setWordWrap(options.contains(MemoOptions::WORD_WRAP)
                                 ? options[MemoOptions::WORD_WRAP].toBool() : AppSettings::instance().memoWordWrap);

    if (options.contains(MemoOptions::SPELLCHECK))
        _memoEditor->setSpellcheckLang(options[MemoOptions::SPELLCHECK].toString());

    if (options.contains(MemoOptions::HIGHLIGHTER))
    {
        auto editor = dynamic_cast<TextMemoEditor*>(_memoEditor);
        if (editor) editor->setHighlighterName(options[MemoOptions::HIGHLIGHTER].toString());
    }
}

bool TextMemoTab::canClose()
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

void TextMemoTab::showMemo()
{
    _memoEditor->showMemo();

    _titleEditor->setText(_memo->title());
    _titleEditor->setModified(false);

    setWindowTitle(_memo->title());
}

bool TextMemoTab::isModified() const
{
    return _memoEditor->isModified() || _titleEditor->isModified();
}

void TextMemoTab::beginEdit()
{
    toggleEditMode(true);
    _memoEditor->beginEdit();

    if (_memo->data().isEmpty())
    {
        _titleEditor->setFocus();
        _titleEditor->selectAll();
    }

    emit onReadOnly(false);
}

void TextMemoTab::cancelEdit()
{
    toggleEditMode(false);
    _memoEditor->endEdit();
    showMemo();
    emit onReadOnly(true);
}

bool TextMemoTab::saveEdit()
{
    MemoUpdateParam update;
    update.title = _titleEditor->text().trimmed();
    update.data = _memoEditor->data();

    auto ok = _enot->updateMemo(_memo, update);
    if (!ok) return false;

    _memoEditor->saveEdit();
    _titleEditor->setModified(false);
    setWindowTitle(_memo->title());
    toggleEditMode(false);
    emit onReadOnly(true);
    return true;
}

void TextMemoTab::toggleEditMode(bool on)
{
    _isEditMode = on;

    _propsPanel->setReadOnly(on);

    _actionSave->setVisible(on);
    _actionCancel->setVisible(on);
    _actionEdit->setVisible(!on);

    if (_memo->type() == MemoType::markdown())
    {
        if (on)
        {
            _actionPreview = new QAction(tr("Edit Mode"));
            _actionPreview->setShortcut(Qt::Key_F5);
            _actionPreview->setToolTip(tr("Switch between Preview and Edit mode"));
            connect(_actionPreview, &QAction::triggered, this, &TextMemoTab::togglePreviewMode);

            _previewButton = new QToolButton;
            _previewButton->setObjectName("button_preview");
            _previewButton->setDefaultAction(_actionPreview);
            _previewButton->setFixedWidth(PREVIEW_BUTTON_WIDTH);

            auto firstAction = _toolbar->actions().value(0);
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

    TabHelpers::setTitleEditorReadOnly(_titleEditor, !on);
}

void TextMemoTab::togglePreviewMode()
{
    auto editor = qobject_cast<MarkdownMemoEditor*>(_memoEditor);
    if (!editor) return;

    bool isPreview = editor->isPreviewMode();
    _actionPreview->setText(isPreview ? tr("Edit Mode") : tr("Preview Mode"));
    editor->togglePreviewMode(!isPreview);
}

