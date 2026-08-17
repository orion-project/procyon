#include "GridFilterPanel.h"

#include "core/Enot.h"

#include "helpers/OriLayouts.h"
#include "widgets/OriFlowLayout.h"

#include <QLabel>
#include <QComboBox>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>

//------------------------------------------------------------------------------
//                             TitleFilterEdit
//------------------------------------------------------------------------------

class TitleFilterEdit : public QLineEdit
{
public:
    TitleFilterEdit(std::function<void()> sumbit) : QLineEdit(), _submit(sumbit)
    {
    }

protected:
    void focusOutEvent(QFocusEvent *e) override
    {
        QLineEdit::focusOutEvent(e);
        if (isModified())
        {
            _submit();
            setModified(false);
        }
    }

private:
    std::function<void()> _submit;
};

//------------------------------------------------------------------------------
//                             FilterPropWidget
//------------------------------------------------------------------------------

class FilterPropWidget : public QWidget
{
public:
    FilterPropWidget(const QString& propName, const QString& propValue,
                     Enot *enot, std::function<void()> submit) : QWidget()
    {
        _propName = propName;

        auto label = new QLabel(propName + ':');

        _editor = new QComboBox;
        _editor->addItem(QString());
        for (const auto& value : enot->propValues(propName))
            _editor->addItem(value);
        _editor->setCurrentText(propValue);
        connect(_editor, &QComboBox::currentIndexChanged, this, submit);

        Ori::Layouts::LayoutH({label, _editor}).setMargin(0).useFor(this);
    }

    QString propName() const { return _propName; }
    QString value() const { return _editor->currentText(); }

private:
    QString _propName;
    QComboBox *_editor;
};

//------------------------------------------------------------------------------
//                             GridFilterPanel
//------------------------------------------------------------------------------

using Self = GridFilterPanel;

GridFilterPanel::GridFilterPanel(Enot *enot) : QFrame(), _enot(enot)
{
    setObjectName("filter_panel");

    new Ori::Widgets::FlowLayout(this, 0, 20, 5);

    auto label = new QLabel(tr("Title:"));

    _titleFilter = new TitleFilterEdit(std::bind(&Self::filterChanged, this));
    _titleFilter->setProperty("role", "filter_item");

    layout()->addWidget(
        Ori::Layouts::LayoutH({label, _titleFilter}).setMargin(0).makeWidget());
}

void GridFilterPanel::tryApplyFilters()
{
    if (!isVisible()) return;
    if (!_titleFilter->hasFocus()) return;
    if (!_titleFilter->isModified()) return;
    _titleFilter->setModified(false);
    emit filterChanged();
}

QString GridFilterPanel::titleFilter() const
{
    return _titleFilter->text().trimmed();
}

void GridFilterPanel::setTitleFilter(const QString& value)
{
    _titleFilter->setText(value);
    _titleFilter->setModified(false);
}

QList<QPair<QString, QString>> GridFilterPanel::propFilters() const
{
    QList<QPair<QString, QString>> res;
    for (auto propFilter : _propFilters)
    {
        auto value = propFilter->value();
        if (!value.isEmpty())
            res << qMakePair(propFilter->propName(), value);
    }
    return res;
}

void GridFilterPanel::setPropFilters(const QList<QPair<QString, QString>>& propFilter)
{
    setUpdatesEnabled(false);

    for (auto propFilter : std::as_const(_propFilters))
    {
        layout()->removeWidget(propFilter);
        propFilter->deleteLater();
    }
    _propFilters.clear();

    for (const auto& propFilter : propFilter)
    {
        auto editor = new FilterPropWidget(propFilter.first, propFilter.second,
                                           _enot, std::bind(&Self::filterChanged, this));
        layout()->addWidget(editor);
        _propFilters.append(editor);
    }

    setUpdatesEnabled(true);
}

void GridFilterPanel::focusTitleFilter()
{
    _titleFilter->selectAll();
    _titleFilter->setFocus();
}
