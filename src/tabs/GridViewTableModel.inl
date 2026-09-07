#include "GridViewColumns.h"
#include "core/Enot.h"
#include "core/MemoType.h"

#include <QAbstractTableModel>
#include <QApplication>

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
