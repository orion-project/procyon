#include "MemoType.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "widgets/OriSelectableTile.h"

#include <QApplication>
#include <QHBoxLayout>

MemoType::~MemoType()
{
}

class PlainTextMemoType : public MemoType
{
public:
    const QString name() const override { return QStringLiteral("plain_text"); }
    const char* title() const override { return QT_TRANSLATE_NOOP("MemoType", "Plain Text"); }
    const QIcon& icon() const override { static QIcon icon(iconPath()); return icon; }
    const QString iconPath() const override { return QStringLiteral(":/icon/memo_plain_text"); }
};

class MarkdownMemoType : public MemoType
{
public:
    const QString name() const override { return QStringLiteral("markdown"); }
    const char* title() const override { return QT_TRANSLATE_NOOP("MemoType", "Markdown"); }
    const QIcon& icon() const override { static QIcon icon(iconPath()); return icon; }
    const QString iconPath() const override { return QStringLiteral(":/icon/memo_markdown"); }
};

class RichTextMemoType : public MemoType
{
public:
    const QString name() const override { return QStringLiteral("rich_text"); }
    const char* title() const override { return QT_TRANSLATE_NOOP("MemoType", "Rich Text"); }
    const QIcon& icon() const override { static QIcon icon(iconPath()); return icon; }
    const QString iconPath() const override { return QStringLiteral(":/icon/memo_rich_text"); }
};

MemoType* MemoType::plainText() { static PlainTextMemoType t; return &t; }
MemoType* MemoType::markdown() { static MarkdownMemoType t; return &t; }
MemoType* MemoType::richText() { static RichTextMemoType t; return &t; }

const QList<MemoType*>& MemoType::all()
{
    static QList<MemoType*> types {
        plainText(),
        markdown(),
        richText()
    };
    return types;
}

MemoType* MemoType::findByName(const QString& name)
{
    for (auto t : all())
        if (t->name() == name)
            return t;
    return plainText();
}

MemoType* MemoType::selectFromDlg()
{
    Ori::Widgets::SelectableTileRadioGroup tripTypeGroup;

    auto tripTypeLayout = new QHBoxLayout();
    tripTypeLayout->setContentsMargins(0, 0, 0, 0);
    tripTypeLayout->setSpacing(12);
    for (auto memoType : QVector<MemoType*>({MemoType::plainText(), MemoType::markdown()}))
    {
        auto tile = new Ori::Widgets::SelectableTile;
        tile->setPixmap(memoType->icon().pixmap(48, 48));
        tile->setTitle(memoType->title());
        tile->setData(QVariant::fromValue(reinterpret_cast<void*>(memoType)));
        tile->setTitleStyleSheet("font-size:15px;margin:0 15px 0 15px;");
        tile->selectionFollowsFocus = true;
        tripTypeLayout->addWidget(tile);
        tripTypeGroup.addTile(tile);
    }

    QWidget content;
    Ori::Layouts::LayoutV({tripTypeLayout}).setMargin(0).setSpacing(12).useFor(&content);

    auto dlg = Ori::Dlg::Dialog(&content, false)
                   .withTitle(qApp->tr("Choose Memo Type"))
                   .withContentToButtonsSpacingFactor(3)
                   .withOkSignal(&tripTypeGroup, SIGNAL(doubleClicked(QVariant)));
    if (dlg.exec())
        return reinterpret_cast<MemoType*>(tripTypeGroup.selectedData().value<void*>());
    return nullptr;
}