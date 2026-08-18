#include "GridViewMemoTab.h"

#include "TabHelpers.h"
#include "core/Enot.h"
#include "core/MemoStore.h"
#include "core/MemoType.h"
#include "widgets/GridFilterPanel.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"

#include <QAbstractTableModel>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QTableView>
#include <QToolBar>
#include <QToolButton>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QGroupBox>

using namespace Qt::StringLiterals;

namespace {

enum class ColumnKind { NONE, ID, PROP };

struct ColumnDef
{
    ColumnKind kind = ColumnKind::NONE;
    std::function<QString()> header;
    std::function<QVariant(Memo*)> value;
    QHeaderView::ResizeMode resizeMode = QHeaderView::ResizeToContents;
};

struct PropFormat
{
    template <typename TValue> struct Format
    {
        TValue value;
        bool fullRow = false;
    };
    std::optional<Format<QColor>> backColor;
    std::optional<Format<QColor>> textColor;
    std::optional<Format<bool>> fontB;
    std::optional<Format<bool>> fontI;
    std::optional<Format<bool>> fontU;
    std::optional<Format<bool>> fontS;

    bool isEmpty() const
    {
        return !backColor && !textColor && !fontB && !fontI && !fontU && !fontS;
    }
};

using PropFormats = QHash<QString, QHash<QString, PropFormat>>;

}

//------------------------------------------------------------------------------
//                            GridViewTableModel
//------------------------------------------------------------------------------

class GridViewTableModel : public QAbstractTableModel
{
public:
    GridViewTableModel(Memo *memo, QObject *parent) : QAbstractTableModel(parent)
    {
        _self = memo;
        _folder = memo->parent();
    }

    int rowCount(const QModelIndex&) const override
    {
        return _folder->memos().size();
    }

    int columnCount(const QModelIndex&) const override
    {
        return _columnDefs.size();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if (role == Qt::DisplayRole)
        {
            switch (orientation)
            {
            case Qt::Vertical:
                return section + 1;
            case Qt::Horizontal:
                return _columnDefs.at(section).header();
            }
        }
        return QVariant();
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid()) return QVariant();

        const auto& memo = _folder->memos().at(index.row());
        const auto& column = _columnDefs.at(index.column());

        if (role == Qt::DecorationRole)
        {
            if (column.kind == ColumnKind::ID)
                return memo->type()->icon();
        }
        else if (role == Qt::ToolTipRole)
        {
            if (column.kind == ColumnKind::ID)
                return memo->type()->title();
        }
        else if (role == Qt::DisplayRole)
        {
            return column.value(memo);
        }

        return QVariant();
    }

    void itemCreating(Entry* entry, int index)
    {
        if (entry->isMemo() && entry->parent() == _folder)
        {
            _isRowCountChanging = true;
            beginInsertRows(QModelIndex(), index, index);
        }
    }

    void itemCreated(Entry* entry)
    {
        if (_isRowCountChanging)
        {
            _isRowCountChanging = false;
            endInsertRows();
        }
    }

    void itemRemoving(Entry* entry)
    {
        if (entry->isMemo() && entry->parent() == _folder)
        {
            _isRowCountChanging = true;
            int index = _folder->memos().indexOf(entry);
            beginRemoveRows(QModelIndex(), index, index);
        }
    }

    void itemRemoved(Entry* entry)
    {
        if (_isRowCountChanging)
        {
            _isRowCountChanging = false;
            endRemoveRows();
        }
    }

    QStringList propColumns() const
    {
        QStringList columns;
        for (const auto& colDef : _columnDefs)
            if (colDef.kind == ColumnKind::PROP)
                columns << colDef.header();
        return columns;
    }

    void setPropColumns(const QStringList& propNames)
    {
        _columnDefs.clear();
        _columnDefs << ColumnDef {
            .kind = ColumnKind::ID,
            .header = []{ return qApp->tr("ID"); },
            .value = [](Memo* memo){ return memo->id(); },
        };
        _columnDefs << ColumnDef {
            .header = []{ return qApp->tr("Title"); },
            .value = [](Memo* memo){ return memo->title(); },
            .resizeMode = QHeaderView::Stretch
        };
        for (const auto& propName : propNames)
        {
            _columnDefs << ColumnDef {
                .kind = ColumnKind::PROP,
                .header = [propName]{ return propName; },
                .value = [propName](Memo* memo){ return memo->props().value(propName); },
            };
        }
        _columnDefs << ColumnDef {
            .header = []{ return qApp->tr("Updated"); },
            .value = [](Memo* memo){ return memo->updated(); },
        };
    }

    void reset()
    {
        beginResetModel();
        endResetModel();
    }

    const QList<ColumnDef>& columnDefs() const { return _columnDefs; }

private:
    Memo *_self;
    Folder *_folder;
    bool _isRowCountChanging = false;
    QList<ColumnDef> _columnDefs;
};

//------------------------------------------------------------------------------
//                            GridViewFilterModel
//------------------------------------------------------------------------------

class GridViewFilterModel : public QSortFilterProxyModel
{
public:
    GridViewFilterModel(Memo *memo, QObject *parent) : QSortFilterProxyModel(parent)
    {
        _folder = memo->parent();
    }

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override
    {
        auto memo = _folder->memos().at(sourceRow);

        if (memo->type() == MemoType::gridView())
            return false;

        if (!_titleFilter.isEmpty())
            if (!memo->title().contains(_titleFilter, Qt::CaseInsensitive))
                return false;

        if (!_propsFilters.isEmpty())
        {
            const auto& memoProps = memo->props();
            for (const auto& filter : std::as_const(_propsFilters))
            {
                if (filter.second.isEmpty())
                    continue;
                if (!memoProps.contains(filter.first))
                    return false;
                if (memoProps.value(filter.first) != filter.second)
                    return false;
            }
        }

        return true;
    }

    void setFilters(const QString& title, const QList<QPair<QString, QString>>& props)
    {
        beginResetModel();
        _titleFilter = title;
        _propsFilters = props;
        endResetModel();
    }

    void setPropFilters(const QList<QPair<QString, QString>>& props)
    {
        beginResetModel();
        _propsFilters = props;
        endResetModel();
    }

private:
    Folder *_folder;
    QString _titleFilter;
    QList<QPair<QString, QString>> _propsFilters;
};

//------------------------------------------------------------------------------
//                            GridViewItemDelegate
//------------------------------------------------------------------------------

class GridViewItemDelegate : public QStyledItemDelegate
{
public:
    GridViewItemDelegate(GridViewMemoTab *gridView) : QStyledItemDelegate(gridView), _gridView(gridView)
    {
        PropFormat solved;
        solved.backColor = { .value = QColor(0, 255, 0, 50), .fullRow = true };
        PropFormat closed;
        closed.backColor = { .value = QColor(0, 0, 0, 50), .fullRow = true };
        QHash<QString, PropFormat> statusFormats;
        statusFormats.insert("Solved", solved);
        statusFormats.insert("Closed", closed);
        _propFormats.insert("Status", statusFormats);
    }

    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override
    {
        QStyledItemDelegate::initStyleOption(option, index);

        const int curCol = index.column();

        Memo* memo = _gridView->memoAtIndex(index);

        const auto& colDefs = _gridView->_tableModel->columnDefs();
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
            QCheckBox *fontB = new QCheckBox(tr("Bold"));
            QCheckBox *fontFullB = new QCheckBox(tr("Full row"));
            QCheckBox *fontI = new QCheckBox(tr("Italic"));
            QCheckBox *fontFullI = new QCheckBox(tr("Full row"));
            QCheckBox *fontU = new QCheckBox(tr("Underline"));
            QCheckBox *fontFullU = new QCheckBox(tr("Full row"));
            QCheckBox *fontS = new QCheckBox(tr("Strikeout"));
            QCheckBox *fontFullS = new QCheckBox(tr("Full row"));

            void apply(PropFormats &propFormats)
            {
                PropFormat fmt;
                if (fontB->isChecked())
                    fmt.fontB = { .value = true, .fullRow = fontFullB->isChecked() };
                if (fontI->isChecked())
                    fmt.fontI = { .value = true, .fullRow = fontFullI->isChecked() };
                if (fontU->isChecked())
                    fmt.fontU = { .value = true, .fullRow = fontFullU->isChecked() };
                if (fontS->isChecked())
                    fmt.fontS = { .value = true, .fullRow = fontFullS->isChecked() };
                propFormats[propName][propValue] = fmt;
            }

            void populate(PropFormats &propFormats)
            {
                propName = nameSelector->currentText();
                propValue = valueSelector->currentText();
                const auto& fmt = propFormats[propName][propValue];
                fontB->setChecked(fmt.fontB.has_value());
                fontFullB->setChecked(fmt.fontB && fmt.fontB->fullRow);
                fontI->setChecked(fmt.fontI.has_value());
                fontFullI->setChecked(fmt.fontI && fmt.fontI->fullRow);
                fontU->setChecked(fmt.fontU.has_value());
                fontFullU->setChecked(fmt.fontU && fmt.fontU->fullRow);
                fontS->setChecked(fmt.fontS.has_value());
                fontFullS->setChecked(fmt.fontS && fmt.fontS->fullRow);
            }
        } c;

        for (const auto& propName : _gridView->_enot->propNames())
            c.nameSelector->addItem(propName);

        auto fillPropValues = [this, &c]{
            auto propName = c.nameSelector->currentText();
            c.valueSelector->clear();
            for (const auto& propValue : _gridView->_enot->propValues(propName))
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

        auto fmtGroup = new QGroupBox(tr("Format"));
        auto fmtLayout = new QGridLayout(fmtGroup);
        int row = 0;
        fmtLayout->addWidget(c.fontB, row, 0);
        fmtLayout->addWidget(c.fontFullB, row, 1);
        row++;
        fmtLayout->addWidget(c.fontI, row, 0);
        fmtLayout->addWidget(c.fontFullI, row, 1);
        row++;
        fmtLayout->addWidget(c.fontU, row, 0);
        fmtLayout->addWidget(c.fontFullU, row, 1);
        row++;
        fmtLayout->addWidget(c.fontS, row, 0);
        fmtLayout->addWidget(c.fontFullS, row, 1);

        auto w = Ori::Layouts::LayoutV({
            Ori::Layouts::LayoutH({
                tr("Property:"), c.nameSelector,
                Ori::Layouts::SpaceH(2),
                tr("Value:"), c.valueSelector,
            }).makeGroupBox(tr("Condition")),
            fmtGroup
        }).makeWidgetAuto();

        auto dlg = Ori::Dlg::Dialog(w)
            .withContentToButtonsSpacingFactor(2);
        if (dlg.exec())
            return true;

        return false;
    }

private:
    PropFormats _propFormats;
    GridViewMemoTab *_gridView;
};

//------------------------------------------------------------------------------
//                             GridViewMemoTab
//------------------------------------------------------------------------------

typedef GridViewMemoTab Self;

GridViewMemoTab::GridViewMemoTab(Enot* enot, Memo* memo) : MemoTab(enot, memo)
{
    _titleEditor = TabHelpers::makeTitleEditor();

    _toolbar = TabHelpers::makeHeaderToolBar();

    _toolMenu = new QMenu(this);
    _toolMenu->addAction(tr("Show Properties..."), this, &Self::chooseColumns);
    _toolMenu->addAction(tr("Property Formats..."), this, &Self::configurePropFormats);
    _toolMenu->addSeparator();
    auto actionFilter = _toolMenu->addAction(tr("Show Filters"), this, &Self::showFilterPanel);
    actionFilter->setShortcut(QKeySequence(Qt::ControlModifier | Qt::Key_F));
    _toolMenu->addAction(tr("Clear Filters"), this, &Self::clearFilters);

    auto toolMenuButton = new QToolButton;
    toolMenuButton->setPopupMode(QToolButton::InstantPopup);
    toolMenuButton->setToolTip(tr("Options"));
    toolMenuButton->setIcon(QIcon(":/toolbar/menu"));
    toolMenuButton->setMenu(_toolMenu);

    _actionEdit = _toolbar->addAction(QIcon(":/toolbar/edit"), tr("Edit"), this, &Self::beginEdit);
    _actionSave = _toolbar->addAction(QIcon(":/toolbar/apply"), tr("Save"), this, &Self::saveEdit);
    _actionCancel = _toolbar->addAction(QIcon(":/toolbar/cancel"), tr("Cancel"), this, &Self::cancelEdit);
    _actionEdit->setShortcut(QKeySequence(Qt::Key_Return, Qt::Key_Return));
    _actionSave->setShortcut(QKeySequence::Save);
    _actionCancel->setShortcut(QKeySequence(Qt::Key_Escape, Qt::Key_Escape));
    _toolbar->addSeparator();
    _toolbar->addAction(QIcon(":/icon/memo_plain_text"), tr("New Memo"), this, &Self::createMemo);
    _toolbar->addSeparator();
    _toolbar->addWidget(toolMenuButton);
    _toolbar->addSeparator();
    _toolbar->addAction(QIcon(":/toolbar/close"), tr("Close Tab"), [this](){
        if (canClose()) deleteLater();
    });

    _contextMenu = new QMenu;
    auto actionOpen = _contextMenu->addAction(tr("Open"), Qt::Key_Return, this, &Self::openSelectedMemo);

    auto toolPanel = TabHelpers::makeHeaderPanel({_titleEditor, _toolbar});

    _tableModel = new GridViewTableModel(memo, this);
    connect(_enot, &Enot::entryCreating, _tableModel, &GridViewTableModel::itemCreating);
    connect(_enot, &Enot::entryCreated, _tableModel, &GridViewTableModel::itemCreated);
    connect(_enot, &Enot::entryDeleted, _tableModel, &GridViewTableModel::itemRemoved);
    connect(_enot, &Enot::entryDeleting, _tableModel, &GridViewTableModel::itemRemoving);
    connect(_enot, &Enot::entryDeleted, _tableModel, &GridViewTableModel::itemRemoved);

    _filterModel = new GridViewFilterModel(memo, this);
    _filterModel->setSourceModel(_tableModel);

    _itemDelegate = new GridViewItemDelegate(this);

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
    auto memoType = MemoType::selectFromDlg();
    if (!memoType) return;

    _enot->createMemo(_memo->parent(), memoType);
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
        emit memoOpenRequested(memo);
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
}










