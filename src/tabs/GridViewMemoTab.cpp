#include "GridViewMemoTab.h"

#include "TabHelpers.h"
#include "core/Enot.h"
#include "core/MemoStore.h"
#include "core/MemoType.h"
#include "widgets/GridFilterPanel.h"
#include "widgets/MemoFactory.h"

#include "helpers/OriLayouts.h"

#include <QApplication>
#include <QSortFilterProxyModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QTableView>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QGridLayout>

using namespace Qt::StringLiterals;

#include "GridViewTableModel.inl"
#include "GridViewFilterModel.inl"
#include "GridViewItemDelegate.inl"

//------------------------------------------------------------------------------
//                             GridViewMemoTab
//------------------------------------------------------------------------------

typedef GridViewMemoTab Self;

GridViewMemoTab::GridViewMemoTab(Enot* enot, Memo* memo) : MemoTab(enot, memo)
{
    _titleEditor = TabHelpers::makeTitleEditor();

    _toolbar = TabHelpers::makeHeaderToolBar();

    _toolMenu = new QMenu(this);
    _toolMenu->addAction(QIcon(":/toolbar/columns"), tr("Show Properties..."), this, &Self::chooseColumns);
    _toolMenu->addAction(QIcon(":/toolbar/brush"), tr("Property Formats..."), this, &Self::configurePropFormats);
    _toolMenu->addSeparator();
    auto actionFilter = _toolMenu->addAction(QIcon(":/toolbar/filter"), tr("Show Filters"), this, &Self::showFilterPanel);
    actionFilter->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_F));
    _toolMenu->addAction(QIcon(":/toolbar/trash"), tr("Clear Filters"), this, &Self::clearFilters);

    auto toolMenuButton = TabHelpers::makeMenuButton(_toolMenu, tr("Options"));

    _addMemoMenu = new QMenu(this);
    for (auto memoType : MemoType::all())
    {
        // No reason for quick-making grid-views from grid-views
        if (memoType == MemoType::gridView()) continue;

        auto action = _addMemoMenu->addAction(memoType->icon(), tr("New: %1").arg(memoType->title()));
        action->setData(memoType->name());
        connect(action, &QAction::triggered, this, &Self::createMemo);
    }

    _addMemoButton = new QToolButton;
    _addMemoButton->setMenu(_addMemoMenu);
    _addMemoButton->setPopupMode(QToolButton::MenuButtonPopup);

    _actionEdit = _toolbar->addAction(QIcon(":/toolbar/edit"), tr("Edit"), this, &Self::beginEdit);
    _actionSave = _toolbar->addAction(QIcon(":/toolbar/apply"), tr("Save"), this, &Self::saveEdit);
    _actionCancel = _toolbar->addAction(QIcon(":/toolbar/cancel"), tr("Cancel"), this, &Self::cancelEdit);
    _actionEdit->setShortcut(QKeySequence(Qt::Key_Return, Qt::Key_Return));
    _actionSave->setShortcut(QKeySequence::Save);
    _actionCancel->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_Escape));
    _toolbar->addSeparator();
    _toolbar->addWidget(_addMemoButton);
    _toolbar->addSeparator();
    _toolbar->addWidget(toolMenuButton);
    _toolbar->addSeparator();
    _toolbar->addAction(QIcon(":/toolbar/close"), tr("Close Tab"), [this](){
        if (canClose()) deleteLater();
    });

    _contextMenu = new QMenu;
    auto actionOpen = _contextMenu->addAction(QIcon(":/toolbar/open"), tr("Open"), Qt::Key_Return, this, &Self::openSelectedMemo);

    auto toolPanel = TabHelpers::makeHeaderPanel({_titleEditor, _toolbar});

    _tableModel = new GridViewTableModel(memo, this);
    connect(_enot, &Enot::entryCreating, _tableModel, &GridViewTableModel::itemCreating);
    connect(_enot, &Enot::entryCreated, _tableModel, &GridViewTableModel::itemCreated);
    connect(_enot, &Enot::entryDeleted, _tableModel, &GridViewTableModel::itemRemoved);
    connect(_enot, &Enot::entryDeleting, _tableModel, &GridViewTableModel::itemRemoving);
    connect(_enot, &Enot::entryDeleted, _tableModel, &GridViewTableModel::itemRemoved);

    _filterModel = new GridViewFilterModel(memo, this);
    _filterModel->setSourceModel(_tableModel);

    _itemDelegate = new GridViewItemDelegate(_enot, this);
    _itemDelegate->memoAtIndex = [this](const QModelIndex& index){ return memoAtIndex(index); };
    // NB: Below, provide the return value explicitly, otherwise constness get lost 
    // and the lambda returns a references to some local copy of the list, and the app can crash
    _itemDelegate->columnDefs = [this]()->const QList<ColumnDef>&{ return _tableModel->columnDefs(); };

    _tableView = new QTableView;
    _tableView->setModel(_filterModel);
    _tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    _tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    _tableView->verticalHeader()->setVisible(false);
    _tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    _tableView->addAction(actionOpen);
    _tableView->setSortingEnabled(true);
    _tableView->setItemDelegate(_itemDelegate);
    connect(_tableView, &QTableView::doubleClicked, this, &Self::openSelectedMemo);
    connect(_tableView, &QTableView::customContextMenuRequested, this, &Self::showContextMenu);

    auto h = _tableView->horizontalHeader();
    h->setMinimumSectionSize(32);
    h->setHighlightSections(false);
    connect(h, &QHeaderView::sectionClicked, this, &Self::saveSortMode);

    _filterPanel = new GridFilterPanel(_enot);
    _filterPanel->setVisible(false);
    connect(_filterPanel, &GridFilterPanel::filterChanged, this, &Self::applyFilters);

    Ori::Layouts::LayoutV({toolPanel, _filterPanel, _tableView}).setMargin(0).setSpacing(0).useFor(this);

    showMemo();
    toggleEditMode(false);
}

void GridViewMemoTab::showMemo()
{
    _titleEditor->setText(_memo->title());

    auto config = Store::memos()->selectOptions(_memo->id());

    QString sortOption = config.value(u"sort"_s).toString();
    int sortColumn = qAbs(sortOption.toInt());
    auto sortOrder = sortOption.startsWith('-') ? Qt::DescendingOrder : Qt::AscendingOrder;
    _filterModel->sort(sortColumn, sortOrder);

    QStringList propColumns;
    QString columnOption = config.value(u"columns"_s).toString();
    for (const auto& prop : QJsonDocument::fromJson(columnOption.toUtf8()).array())
        propColumns << prop.toString();
    applyColumns(propColumns);

    QString filterOption = config.value(u"filter"_s).toString();
    auto filtersJson = QJsonDocument::fromJson(filterOption.toUtf8()).object();
    auto titleFilter = filtersJson["title"_L1].toString();
    bool hasFilters = !titleFilter.isEmpty();
    _filterPanel->setTitleFilter(titleFilter);
    auto propFiltersJson = filtersJson["props"_L1].toObject();
    QList<QPair<QString, QString>> propFilters;
    for (const auto& propName : std::as_const(propColumns))
    {
        auto value = propFiltersJson[propName].toString();
        if (!value.isEmpty()) hasFilters = true;
        propFilters << qMakePair(propName, value);
    }
    if (hasFilters)
    {
        _filterPanel->setVisible(true);
        _filterPanel->setPropFilters(propFilters);
        _filterModel->setFilters(titleFilter, propFilters);
    }

    QString newMemoType = config.value(u"new_memo_type"_s).toString();
    auto newMemoActions = _addMemoMenu->actions();
    for (auto action : std::as_const(newMemoActions))
        if (action->data().toString() == newMemoType)
        {
            _addMemoButton->setDefaultAction(action);
            break;
        }
    if (!_addMemoButton->defaultAction() && !newMemoActions.isEmpty())
        _addMemoButton->setDefaultAction(newMemoActions.first());
        
    _itemDelegate->loadPropFormats(config.value(u"prop_formats"_s).toString());

    setWindowTitle(_memo->title());
}

void GridViewMemoTab::beginEdit()
{
    toggleEditMode(true);

    _titleEditor->setFocus();
    _titleEditor->selectAll();
}

void GridViewMemoTab::cancelEdit()
{
    toggleEditMode(false);
    _titleEditor->setText(_memo->title());
}

bool GridViewMemoTab::saveEdit()
{
    MemoUpdateParam update;
    update.title = _titleEditor->text().trimmed();

    auto ok = _enot->updateMemo(_memo, update);
    if (!ok) return false;

    setWindowTitle(_memo->title());
    toggleEditMode(false);
    return true;
}

void GridViewMemoTab::toggleEditMode(bool on)
{
    _actionSave->setVisible(on);
    _actionCancel->setVisible(on);
    _actionEdit->setVisible(!on);

    TabHelpers::setTitleEditorReadOnly(_titleEditor, !on);
}

void GridViewMemoTab::createMemo()
{
    QAction* action = dynamic_cast<QAction*>(sender());
    if (!action) return;

    auto memoType = MemoType::findByName(action->data().toString());
    if (!memoType) return;

    MemoFactory::createMemo(_enot, _memo->parent(), memoType);

    if (_addMemoButton->defaultAction() != action)
    {
        _addMemoButton->setDefaultAction(action);
        Store::memos()->updateOption(_memo->id(), u"new_memo_type"_s, memoType->name());
    }
}

Memo* GridViewMemoTab::selectedMemo() const
{
    QModelIndexList selection = _tableView->selectionModel()->selectedRows();
    if (selection.empty()) return nullptr;
    return memoAtIndex(selection.at(0));
}

Memo* GridViewMemoTab::memoAtIndex(const QModelIndex& index) const
{
    int row = _filterModel->mapToSource(index).row();
    return _memo->parent()->memos().at(row);
}

void GridViewMemoTab::showContextMenu(const QPoint& pos)
{
    if (selectedMemo())
        _contextMenu->popup(_tableView->mapToGlobal(pos));
}

void GridViewMemoTab::openSelectedMemo()
{
    if (!_tableView->hasFocus())
    {
        _filterPanel->tryApplyFilters();
        return;
    }

    auto memo = selectedMemo();
    if (memo)
        emit memoOpenRequested(memo->id());
}

void GridViewMemoTab::applyFilters()
{
    auto titleFilter = _filterPanel->titleFilter();
    auto propFilters = _filterPanel->propFilters();
    _filterModel->setFilters(titleFilter, propFilters);

    QJsonObject filterJson;
    if (!titleFilter.isEmpty())
        filterJson["title"_L1] = titleFilter;
    if (!propFilters.isEmpty())
    {
        QJsonObject propsJson;
        for (const auto& filter : std::as_const(propFilters))
            propsJson[filter.first] = filter.second;
        filterJson["props"_L1] = propsJson;
    }
    Store::memos()->updateOption(_memo->id(), u"filter"_s,
        QJsonDocument(filterJson).toJson(QJsonDocument::Compact));
}

void GridViewMemoTab::applyColumns(const QStringList &propNames)
{
    _tableModel->setPropColumns(propNames);
    _tableModel->reset();

    auto h = _tableView->horizontalHeader();
    const auto& cols = _tableModel->columnDefs();
    for (int i = 0; i < cols.size(); i++)
        h->setSectionResizeMode(i, cols.at(i).resizeMode);
}

void GridViewMemoTab::chooseColumns()
{
    auto w = Ori::Layouts::LayoutV({}).makeWidgetAuto();

    QList<QCheckBox*> flags;
    auto curColumns = _tableModel->propColumns();
    for (const auto& propName : _enot->propNames())
    {
        auto flag = new QCheckBox(propName);
        flag->setChecked(curColumns.contains(propName));
        w->layout()->addWidget(flag);
        flags << flag;
    }

    if (!Ori::Dlg::Dialog(w).exec()) return;

    QStringList newColumns;
    for (auto flag : std::as_const(flags))
        if (flag->isChecked())
            newColumns << flag->text();
    applyColumns(newColumns);

    Store::memos()->updateOption(_memo->id(), "columns",
        QJsonDocument(QJsonArray::fromStringList(newColumns)).toJson(QJsonDocument::Compact));

    // Reinitialize filters
    if (_filterPanel->isVisible())
    {
        auto oldPropFilters = _filterPanel->propFilters();
        QList<QPair<QString, QString>> newPropFilters;
        for (const auto& propName : std::as_const(newColumns))
        {
            auto newFilter = qMakePair(propName, QString());
            for (const auto& oldFilter : std::as_const(oldPropFilters))
                if (oldFilter.first == propName)
                {
                    newFilter.second = oldFilter.second;
                    break;
                }
            newPropFilters << newFilter;
        }
        _filterPanel->setPropFilters(newPropFilters);
        applyFilters();
    }
}

void GridViewMemoTab::saveSortMode()
{
    QString value = QString::number(_filterModel->sortColumn());
    if (_filterModel->sortOrder() == Qt::DescendingOrder)
        value = '-' + value;
    Store::memos()->updateOption(_memo->id(), "sort", value);
}

void GridViewMemoTab::showFilterPanel()
{
    if (!_filterPanel->isVisible())
    {
        QList<QPair<QString, QString>> propFilters;
        auto propColumns = _tableModel->propColumns();
        for (const auto& propName : std::as_const(propColumns))
            propFilters << qMakePair(propName, QString());
        _filterPanel->setPropFilters(propFilters);
        _filterPanel->setTitleFilter({});
        _filterPanel->show();
    }
    _filterPanel->focusTitleFilter();
}

void GridViewMemoTab::clearFilters()
{
    if (!_filterPanel->isVisible()) return;

    _filterPanel->hide();
    _filterModel->setFilters({}, {});
    Store::memos()->updateOption(_memo->id(), u"filter"_s, QString());
}

void GridViewMemoTab::configurePropFormats()
{
    if (!_itemDelegate->configureFormats())
        return;
    _enot->updateMemoOption(_memo->id(), u"prop_formats"_s, _itemDelegate->propFormatsStr());
    _tableView->repaint();
}










