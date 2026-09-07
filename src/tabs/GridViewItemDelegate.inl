#include "GridViewColumns.h"
#include "core/Enot.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "widgets/OriColorSelectors.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QJsonObject>
#include <QStyledItemDelegate>

using namespace Qt::StringLiterals;

class GridViewItemDelegate : public QStyledItemDelegate
{
public:
    std::function<Memo*(const QModelIndex&)> memoAtIndex;
    std::function<const QList<ColumnDef>&()> columnDefs;

    GridViewItemDelegate(Enot *enot, QObject *parent) : QStyledItemDelegate(parent), _enot(enot)
    {
    }

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);

        const int curCol = index.column();

        Memo* memo = memoAtIndex(index);
        const auto& colDefs = columnDefs();
        const int colCount = colDefs.size();
        for (int col = 0; col < colCount; col++)
        {
            const auto& colDef = colDefs.at(col);
            if (colDef.kind != ColumnKind::PROP)
                continue;

            QString propName = colDef.header();
            if (!_propFormats.contains(propName))
                continue;

            const auto& propValues = memo->props();
            if (!propValues.contains(propName))
                continue;

            const auto& propValue = propValues.value(propName);
            const auto& valueFormats = _propFormats.value(propName);
            if (!valueFormats.contains(propValue))
                continue;

            const auto& fmt = valueFormats.value(propValue);
            if (fmt.backColor && (fmt.backColor->fullRow || col == curCol))
                option->backgroundBrush = fmt.backColor->value;
            if (fmt.textColor && (fmt.textColor->fullRow || col == curCol))
                option->palette.setBrush(QPalette::Text, fmt.textColor->value);
            if (fmt.fontB && (fmt.fontB->fullRow || col == curCol))
                option->font.setBold(true);
            if (fmt.fontI && (fmt.fontI->fullRow || col == curCol))
                option->font.setItalic(true);
            if (fmt.fontU && (fmt.fontU->fullRow || col == curCol))
                option->font.setUnderline(true);
            if (fmt.fontS && (fmt.fontS->fullRow || col == curCol))
                option->font.setStrikeOut(true);
        }
    }

    void setPropFormats(const PropFormats& formats) { _propFormats = formats; }

    bool configureFormats()
    {
        PropFormats formats = _propFormats;

        struct
        {
            QString propName;
            QString propValue;
            QComboBox *nameSelector = new QComboBox;
            QComboBox *valueSelector = new QComboBox;
            QCheckBox *fontB1 = new QCheckBox(tr("Bold"));
            QCheckBox *fontB0 = new QCheckBox(tr("Full row"));
            QCheckBox *fontI1 = new QCheckBox(tr("Italic"));
            QCheckBox *fontI0 = new QCheckBox(tr("Full row"));
            QCheckBox *fontU1 = new QCheckBox(tr("Underline"));
            QCheckBox *fontU0 = new QCheckBox(tr("Full row"));
            QCheckBox *fontS1 = new QCheckBox(tr("Strikeout"));
            QCheckBox *fontS0 = new QCheckBox(tr("Full row"));
            QCheckBox *colorB1 = new QCheckBox(tr("Back color"));
            QCheckBox *colorB0 = new QCheckBox(tr("Full row"));
            QCheckBox *colorT1 = new QCheckBox(tr("Text color"));
            QCheckBox *colorT0 = new QCheckBox(tr("Full row"));
            Ori::Widgets::ColorButton *colorB = new Ori::Widgets::ColorButton;
            Ori::Widgets::ColorButton *colorT = new Ori::Widgets::ColorButton;

            void apply(PropFormats &propFormats)
            {
                ValueFormat fmt;
                if (fontB1->isChecked())
                    fmt.fontB = { .value = true, .fullRow = fontB0->isChecked() };
                if (fontI1->isChecked())
                    fmt.fontI = { .value = true, .fullRow = fontI0->isChecked() };
                if (fontU1->isChecked())
                    fmt.fontU = { .value = true, .fullRow = fontU0->isChecked() };
                if (fontS1->isChecked())
                    fmt.fontS = { .value = true, .fullRow = fontS0->isChecked() };
                if (colorB1->isChecked())
                    fmt.backColor = { .value = colorB->selectedColor(), .fullRow = colorB0->isChecked() };
                if (colorT1->isChecked())
                    fmt.textColor = { .value = colorT->selectedColor(), .fullRow = colorT0->isChecked() };
                propFormats[propName][propValue] = fmt;
            }

            void populate(PropFormats &propFormats)
            {
                propName = nameSelector->currentText();
                propValue = valueSelector->currentText();
                const auto& fmt = propFormats[propName][propValue];
                fontB1->setChecked(fmt.fontB.has_value());
                fontB0->setChecked(fmt.fontB && fmt.fontB->fullRow);
                fontI1->setChecked(fmt.fontI.has_value());
                fontI0->setChecked(fmt.fontI && fmt.fontI->fullRow);
                fontU1->setChecked(fmt.fontU.has_value());
                fontU0->setChecked(fmt.fontU && fmt.fontU->fullRow);
                fontS1->setChecked(fmt.fontS.has_value());
                fontS0->setChecked(fmt.fontS && fmt.fontS->fullRow);
                colorB1->setChecked(fmt.backColor.has_value());
                colorB0->setChecked(fmt.backColor && fmt.backColor->fullRow);
                colorT1->setChecked(fmt.textColor.has_value());
                colorT0->setChecked(fmt.textColor && fmt.textColor->fullRow);
                colorB->setEnabled(colorB1->isChecked());
                colorT->setEnabled(colorT1->isChecked());
                if (fmt.backColor) colorB->setSelectedColor(fmt.backColor->value);
                if (fmt.textColor) colorT->setSelectedColor(fmt.textColor->value);
            }
        } c;

        c.colorB->drawIconFrame = false;
        c.colorT->drawIconFrame = false;
        c.colorB->setProperty("role", "color_selector");
        c.colorT->setProperty("role", "color_selector");
        c.colorB->setToolTip(qApp->tr("Select back color"));
        c.colorT->setToolTip(qApp->tr("Select text color"));
        c.colorB->setSelectedColor(Qt::white);
        c.colorT->setSelectedColor(Qt::black);
        connect(c.colorB1, &QCheckBox::clicked, [&c]{ c.colorB->setEnabled(c.colorB1->isChecked()); });
        connect(c.colorT1, &QCheckBox::clicked, [&c]{ c.colorT->setEnabled(c.colorT1->isChecked()); });

        for (const auto& propName : _enot->propNames())
            c.nameSelector->addItem(propName);

        auto fillPropValues = [this, &c]{
            auto propName = c.nameSelector->currentText();
            c.valueSelector->clear();
            for (const auto& propValue : _enot->propValues(propName))
                c.valueSelector->addItem(propValue);
        };

        connect(c.nameSelector, &QComboBox::currentIndexChanged, this, fillPropValues);
        fillPropValues();

        auto fillPropFormats = [this, &c, &formats]{
            c.apply(formats);
            c.populate(formats);
        };

        connect(c.valueSelector, &QComboBox::currentIndexChanged, this, fillPropFormats);
        c.populate(formats);

        auto w = Ori::Layouts::LayoutV({
            Ori::Layouts::LayoutH({
                tr("Property:"), c.nameSelector,
                Ori::Layouts::SpaceH(2),
                tr("Value:"), c.valueSelector,
            }).makeGroupBox(tr("Condition")),
            Ori::Layouts::Grid({
                { c.fontB1, "", c.fontB0 },
                { c.fontI1, "", c.fontI0 },
                { c.fontU1, "", c.fontU0 },
                { c.fontS1, "", c.fontS0 },
                { c.colorB1, c.colorB, c.colorB0 },
                { c.colorT1, c.colorT, c.colorT0 },
            }).makeGroupBox(tr("Format")),
        }).makeWidgetAuto();

        auto dlg = Ori::Dlg::Dialog(w)
            .withContentToButtonsSpacingFactor(2)
            .withOnDlgShown([&c]{
                const int h = c.colorB1->height();
                const int w = 70;
                const int m = 8;
                for (auto b : {c.colorB, c.colorT})
                {
                    b->setFixedSize({w, h});
                    b->setIconSize({w - m, h - m});
                    b->setIconRect({0, 0, w - m, h - m});
                    b->setSelectedColor(b->selectedColor());
                }
            });
        if (dlg.exec())
        {
            c.apply(formats);
            _propFormats.clear();
            for (auto it = formats.cbegin(); it != formats.cend(); it++)
            {
                QString propName = it.key();
                const PropFormat& propFmt = it.value();
                PropFormat newPropFmt;
                for (auto jt = propFmt.cbegin(); jt != propFmt.cend(); jt++)   
                {
                    QString propValue = jt.key();
                    const ValueFormat& valueFmt = jt.value();
                    if (!valueFmt.isEmpty())
                        newPropFmt[propValue] = valueFmt;
                }
                if (!propFmt.isEmpty())
                    _propFormats[propName] = propFmt;
            }
            return true;
        }

        return false;
    }
    
    void loadPropFormats(const QString& s)
    {
        _propFormats.clear();
        if (s.isEmpty())
            return;
        auto root = QJsonDocument::fromJson(s.toUtf8()).object();
        for (auto it = root.constBegin(); it != root.constEnd(); it++)
        {
            QString propName = it.key();
            QJsonObject propJson = it.value().toObject();
            PropFormat propFmt;
            for (auto jt = propJson.constBegin(); jt != propJson.constEnd(); jt++)   
            {
                QString propValue = jt.key();
                QJsonObject fmtJson = jt.value().toObject();
                ValueFormat valueFmt;
                valueFmt.backColor = ValueFormat::fromJson<QColor>(fmtJson, "backColor"_L1);
                valueFmt.textColor = ValueFormat::fromJson<QColor>(fmtJson, "textColor"_L1);
                valueFmt.fontB = ValueFormat::fromJson<bool>(fmtJson, "fontB"_L1);
                valueFmt.fontI = ValueFormat::fromJson<bool>(fmtJson, "fontI"_L1);
                valueFmt.fontU = ValueFormat::fromJson<bool>(fmtJson, "fontU"_L1);
                valueFmt.fontS = ValueFormat::fromJson<bool>(fmtJson, "fontS"_L1);
                if (!valueFmt.isEmpty())
                    propFmt[propValue] = valueFmt;
            }
            if (!propFmt.isEmpty())
                _propFormats[propName] = propFmt;
        }
    }
    
    QString propFormatsStr() const
    {
        QJsonObject root;
        for (auto it = _propFormats.cbegin(); it != _propFormats.cend(); it++)
        {
            QString propName = it.key();
            const PropFormat& propFmt = it.value();
            QJsonObject propJson;
            for (auto jt = propFmt.cbegin(); jt != propFmt.cend(); jt++)   
            {
                QString propValue = jt.key();
                const ValueFormat& valueFmt = jt.value();
                QJsonObject fmtJson;
                if (valueFmt.backColor)
                    fmtJson["backColor"_L1] = valueFmt.backColor->toJson();
                if (valueFmt.textColor)
                    fmtJson["textColor"_L1] = valueFmt.textColor->toJson();
                if (valueFmt.fontB)
                    fmtJson["fontB"_L1] = valueFmt.fontB->toJson();
                if (valueFmt.fontI)
                    fmtJson["fontI"_L1] = valueFmt.fontI->toJson();
                if (valueFmt.fontU)
                    fmtJson["fontU"_L1] = valueFmt.fontU->toJson();
                if (valueFmt.fontS)
                    fmtJson["fontS"_L1] = valueFmt.fontS->toJson();
                if (!fmtJson.isEmpty())
                    propJson[propValue] = fmtJson;
            }
            if (!propJson.isEmpty())
                root[propName] = propJson;
        }
        if (!root.isEmpty())
            return QString::fromUtf8(QJsonDocument(root).toJson());
        return {};
    }

private:
    Enot *_enot;
    PropFormats _propFormats;
};
