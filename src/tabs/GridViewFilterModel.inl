#include "core/Enot.h"
#include "core/MemoStore.h"
#include "core/MemoType.h"

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>

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
