#include "MemoType.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "widgets/OriSelectableTile.h"

#include <QApplication>
#include <QHBoxLayout>

MemoType::MemoType(const QString& name, const char* title, const QString& iconPath)
{
    _name = name;
    _title = title;
    _icon = QIcon(iconPath);
}

MemoType* MemoType::plainText()
{
    static MemoType t("plain_text", QT_TRANSLATE_NOOP("MemoType", "Plain Text"), ":/icon/memo_plain_text");
    return &t;
}

MemoType* MemoType::markdown()
{
    static MemoType t("markdown", QT_TRANSLATE_NOOP("MemoType", "Markdown"), ":/icon/memo_markdown");
    return &t;
}

MemoType* MemoType::richText()
{
    static MemoType t("rich_text", QT_TRANSLATE_NOOP("MemoType", "Rich Text"), ":/icon/memo_rich_text");
    return &t;
}

MemoType* MemoType::gridView()
{
    static MemoType t("grid_view", QT_TRANSLATE_NOOP("MemoType", "Grid View"), ":/icon/memo_rich_text");
    return &t;
}

const QList<MemoType*>& MemoType::all()
{
    static QList<MemoType*> types { plainText(), markdown(), richText(), gridView() };
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
    for (auto memoType : { plainText(), markdown(), gridView() })
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