#include "GridViewMemoTab.h"

#include "TabHelpers.h"
#include "core/Enot.h"
#include "core/MemoStore.h"
#include "core/MemoType.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"

#include <QAbstractTableModel>
#include <QApplication>
#include <QCheckBox>
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
#include <QVBoxLayout>

namespace {

enum class ColumnKind { NONE, ID };

struct ColumnDef
{
    ColumnKind kind = ColumnKind::NONE;
    std::function<QString()> header;
    std::function<QVariant(Memo*)> value;
    QHeaderView::ResizeMode resizeMode = QHeaderView::ResizeToContents;
};
}

//------------------------------------------------------------------------------
//                            GridViewTableModel
//------------------------------------------------------------------------------

class GridViewTableModel : public QAbstractTableModel
{
public:
    GridViewTableModel(Memo *memo) : QAbstractTableModel()
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
//                           GridViewMemoTab::Config
//------------------------------------------------------------------------------

QString GridViewMemoTab::Config::toString() const
{
    QJsonArray jsonCols;
    for (const auto& propName : propColumns)
        jsonCols.append(propName);

    QJsonObject jsonRoot;
    jsonRoot["propColumns"] = jsonCols;

    return QJsonDocument(jsonRoot).toJson();
}

void GridViewMemoTab::Config::load(const QString& s)
{
    if (s.isEmpty())
        return;

    QJsonParseError jsonErr;
    auto jsonDoc = QJsonDocument::fromJson(s.toUtf8(), &jsonErr);
    if (jsonErr.error != QJsonParseError::NoError)
    {
        qWarning() << "Failed to parse grid config" << jsonErr.errorString();
        return;
    }

    auto jsonRoot = jsonDoc.object();

    auto jsonCols = jsonRoot["propColumns"].toArray();
    for (auto it = jsonCols.cbegin(); it != jsonCols.cend(); it++)
    {
        auto propName = it->toString();
        if (!propName.isEmpty())
            propColumns << propName;
    }
}

//------------------------------------------------------------------------------
//                             GridViewMemoTab
//------------------------------------------------------------------------------

typedef GridViewMemoTab Self;

GridViewMemoTab::GridViewMemoTab(Enot* enot, Memo* memo) : MemoTab(enot, memo)
{
    _titleEditor = TabHelpers::makeTitleEditor();

    _toolbar = TabHelpers::makeHeaderToolBar();

    _toolMenu = new QMenu(this);
    _toolMenu->addAction(tr("Show columns..."), this, &Self::chooseColumns);

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

    _tableModel = new GridViewTableModel(memo);
    connect(_enot, &Enot::entryCreating, _tableModel, &GridViewTableModel::itemCreating);
    connect(_enot, &Enot::entryCreated, _tableModel, &GridViewTableModel::itemCreated);
    connect(_enot, &Enot::entryDeleted, _tableModel, &GridViewTableModel::itemRemoved);
    connect(_enot, &Enot::entryDeleting, _tableModel, &GridViewTableModel::itemRemoving);
    connect(_enot, &Enot::entryDeleted, _tableModel, &GridViewTableModel::itemRemoved);

    _tableView = new QTableView;
    _tableView->setModel(_tableModel);
    _tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    _tableView->setSelectionMode(QAbstractItemView::SingleSelection);
    _tableView->verticalHeader()->setVisible(false);
    _tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    _tableView->addAction(actionOpen);
    connect(_tableView, &QTableView::doubleClicked, this, &Self::openSelectedMemo);
    connect(_tableView, &QTableView::customContextMenuRequested, this, &Self::showContextMenu);

    auto h = _tableView->horizontalHeader();
    h->setMinimumSectionSize(32);
    h->setHighlightSections(false);

    Ori::Layouts::LayoutV({toolPanel, _tableView}).setMargin(0).setSpacing(0).useFor(this);

    showMemo();
    toggleEditMode(false);
}

void GridViewMemoTab::showMemo()
{
    _titleEditor->setText(_memo->title());

    _config.load(_memo->data());
    applyColumns();

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
    showMemo();
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

Entry* GridViewMemoTab::selectedEntry() const
{
    QModelIndexList selection = _tableView->selectionModel()->selectedRows();
    if (selection.empty()) return nullptr;
    int row = selection.at(0).row();
    return _memo->parent()->memos().at(row);
}

void GridViewMemoTab::showContextMenu(const QPoint& pos)
{
    auto entry = selectedEntry();
    if (entry->isMemo())
        _contextMenu->popup(_tableView->mapToGlobal(pos));
}

void GridViewMemoTab::openSelectedMemo()
{
    auto entry = selectedEntry();
    if (entry->isMemo())
        emit memoOpenRequested(entry->asMemo());
}

void GridViewMemoTab::applyColumns()
{
    _tableModel->setPropColumns(_config.propColumns);
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
    for (const auto& propName : _enot->propNames())
    {
        auto flag = new QCheckBox(propName);
        flag->setChecked(_config.propColumns.contains(propName));
        w->layout()->addWidget(flag);
        flags << flag;
    }

    if (!Ori::Dlg::Dialog(w).exec()) return;

    _config.propColumns.clear();
    for (auto flag : std::as_const(flags))
        if (flag->isChecked())
            _config.propColumns << flag->text();
    applyColumns();

    MemoUpdateParam update;
    update.data = _config.toString();
    _enot->updateMemo(_memo, update);
}




























