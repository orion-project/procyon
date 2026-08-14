#include "MemoPropsPanel.h"

#include "core/Enot.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "widgets/OriFlowLayout.h"
#include "widgets/OriLabels.h"
#include "helpers/OriWidgets.h"

#include <QMenu>
#include <QComboBox>

typedef MemoPropsPanel Self;

//------------------------------------------------------------------------------
//                              MemoPropWidget
//------------------------------------------------------------------------------

class MemoPropWidget : public QWidget
{
public:
    MemoPropWidget(const QString& name, const QString& value, std::function<void(const QString&, const QPoint&)> clickHandler)
        : QWidget(), _name(name), _value(value), _clickHandler(clickHandler)
    {
        auto nameLabel = new QLabel(name + ":");
        nameLabel->setProperty("role", "prop_name");

        _readonlyLabel = new QLabel(value);
        _readonlyLabel->setProperty("role", "prop_value");

        Ori::Layouts::LayoutH({nameLabel, _readonlyLabel}).setMargin(0).setSpacing(0).useFor(this);
    }

    QString value() const { return _value; }

    void setValue(const QString& value)
    {
        if (_editableLabel && _editableLabel->isVisible())
            _editableLabel->setText(value);
        else
        {
            _readonlyLabel->setText(value);
            _value = value;
        }
    }

    void switchToEditable()
    {
        if (!_editableLabel)
        {
            _editableLabel = new Ori::Widgets::Label;
            _editableLabel->setProperty("role", "prop_editor");
            _editableLabel->setCursor(Qt::PointingHandCursor);
            connect(_editableLabel, &Ori::Widgets::Label::clicked, this, &MemoPropWidget::editableLabelClicked);
            layout()->addWidget(_editableLabel);
        }
        _editableLabel->setText(_value);
        _readonlyLabel->setVisible(false);
        _editableLabel->setVisible(true);
    }

    void switchToReadOnly()
    {
        if (_editableLabel)
            _editableLabel->setVisible(false);
        _readonlyLabel->setText(_value);
        _readonlyLabel->setVisible(true);
    }

    void apply()
    {
        if (_editableLabel)
        {
            _value = _editableLabel->text();
            _transient = false;
        }
    }

    QString propName() const { return _name; }

    void setTransient() { _transient = true; }
    bool isTransient() const { return _transient; }

private:
    QString _name, _value;
    QLabel *_readonlyLabel;
    Ori::Widgets::Label *_editableLabel = nullptr;
    bool _transient = false;
    std::function<void(const QString&, const QPoint&)> _clickHandler;

    void editableLabelClicked()
    {
        _clickHandler(_name, _editableLabel->mapToGlobal(QPoint(3, _editableLabel->height()-3)));
    }
};

//------------------------------------------------------------------------------
//                              MemoPropsPanel
//------------------------------------------------------------------------------

MemoPropsPanel::MemoPropsPanel(Enot* enot) : QFrame(), _enot(enot)
{
    setObjectName("props_panel");

    new Ori::Widgets::FlowLayout(this, 0, 0, 5);

    _actionAddValue = Ori::Gui::action(tr("Add New Value..."), this, &Self::addNewValue);
    _actionDeleteProp = Ori::Gui::action(tr("Delete Property..."), this, &Self::deleteProp);

    _menu = new QMenu(this);
    connect(_menu, &QMenu::aboutToShow, this, &Self::updateValuesMenu);
}

void MemoPropsPanel::addPropViaDlg()
{
    auto valueEditor = new QComboBox;
    valueEditor->setEditable(true);

    auto nameEditor = new QComboBox;
    nameEditor->setEditable(true);
    for (const auto& propName : _enot->propNames())
        nameEditor->addItem(propName);

    auto fillPropValues = [this, nameEditor, valueEditor]{
        //auto oldText = valueEditor->currentText();
        auto propName = nameEditor->currentText();
        valueEditor->clear();
        for (const auto& propValue : _enot->propValues(propName))
            valueEditor->addItem(propValue);
        //valueEditor->setCurrentText(oldText);
    };

    connect(nameEditor, &QComboBox::currentTextChanged, this, fillPropValues);
    fillPropValues();

    auto widget = Ori::Layouts::LayoutV({
        tr("Name:"), nameEditor,
        tr("Value:"), valueEditor,
    }).makeWidgetAuto();

    auto dlg = Ori::Dlg::Dialog(widget)
        .withContentToButtonsSpacingFactor(2)
        .withVerification([nameEditor, valueEditor]{
            if (nameEditor->currentText().trimmed().isEmpty())
                return tr("Property name must not be empty");
            if (valueEditor->currentText().trimmed().isEmpty())
                return tr("Property value must not be empty");
            return QString();
        });

    if (dlg.exec())
    {
        auto name = nameEditor->currentText().trimmed();
        auto value = valueEditor->currentText().trimmed();
        _enot->addPossiblePropValue(name, value);
        if (_valueViews.contains(name))
        {
            _valueViews[name]->setValue(value);
        }
        else
        {
            addProp(name, value);
            _valueViews[name]->setTransient();
            _valueViews[name]->switchToEditable();
        }
        _enot->addPossiblePropValue(name, value);
    }
}

void MemoPropsPanel::addProp(const QString& name, const QString& value)
{
    auto widget = new MemoPropWidget(name, value, [this](const QString& propName, const QPoint& pos){
        _activeProp = propName;
        _menu->popup(pos);
    });
    _valueViews.insert(name, widget);
    layout()->addWidget(widget);
    _hasValues = true;
    if (!isVisible())
        show();
}

void MemoPropsPanel::switchToEditable()
{
    for (auto view : std::as_const(_valueViews))
        if (view->isVisible())
            view->switchToEditable();
}

void MemoPropsPanel::switchToReadonly()
{
    // Restore "deleted" props when editing is cancelling.
    // If the editing has been applied before switching to read-only,
    // then removed props are cleared alread and nothing happens here
    for (auto view : std::as_const(_removedProps))
    {
        _valueViews.insert(view->propName(), view);
        view->show();
    }
    _removedProps.clear();

    QList<QString> canceledTransient;

    for (auto view : std::as_const(_valueViews))
    {
        if (view->isTransient())
        {
            canceledTransient.append(view->propName());
            continue;
        }

        view->switchToReadOnly();
    }

    for (const auto& name : std::as_const(canceledTransient))
    {
        _valueViews.value(name)->deleteLater();
        _valueViews.remove(name);
    }

    if (!_valueViews.isEmpty() && !isVisible())
        show();
}

void MemoPropsPanel::setReadOnly(bool on)
{
    _isReadonly = on;
    setUpdatesEnabled(false);
    if (on) switchToEditable();
    else switchToReadonly();
    setUpdatesEnabled(true);
}

void MemoPropsPanel::apply()
{
    for (auto view : std::as_const(_removedProps))
        view->deleteLater();
    _removedProps.clear();

    for (auto view : std::as_const(_valueViews))
        view->apply();
}

QHash<QString, QString> MemoPropsPanel::values() const
{
    QHash<QString, QString> res;
    for (auto view : _valueViews)
        res.insert(view->propName(), view->value());
    return res;
}

void MemoPropsPanel::updateValuesMenu()
{
    _menu->clear();
    for (const QString& value : _enot->propValues(_activeProp))
        _menu->addAction(value, _menu, [this, value]{
            if (_valueViews.contains(_activeProp))
                _valueViews[_activeProp]->setValue(value);
        });
    _menu->addSeparator();
    _menu->addAction(_actionAddValue);
    _menu->addAction(_actionDeleteProp);
}

void MemoPropsPanel::addNewValue()
{
    if (!_valueViews.contains(_activeProp))
        return;

    QString newValue = Ori::Dlg::inputText(tr("New value:"), {});
    if (newValue.isEmpty()) return;

    _valueViews[_activeProp]->setValue(newValue);
    _enot->addPossiblePropValue(_activeProp, newValue);
}

void MemoPropsPanel::deleteProp()
{
    if (!Ori::Dlg::yes(tr("Delete property <b>%1</b> from the current memo?").arg(_activeProp)))
        return;
    if (!_valueViews.contains(_activeProp))
        return;
    auto view = _valueViews.value(_activeProp);
    view->hide();
    if (view->isTransient())
        view->deleteLater();
    else
        _removedProps.append(view);
    _valueViews.remove(_activeProp);
    if (_valueViews.isEmpty())
        hide();
}
