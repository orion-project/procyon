#include "OpenTabsWidget.h"

#include "tabs/MemoTab.h"
#include "core/Db.h"

#include "helpers/OriLayouts.h"

#include <QDebug>
#include <QListWidget>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTimer>

namespace {
QImage makeMarker(const QString& path)
{
    return QImage(path).scaled(QSize(24, 24), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

class OpenTabItemDelegate : public QStyledItemDelegate
{
public:
    OpenTabItemDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (index.row() % 2 == 0)
        {
            static QBrush alternateBrush(QColor(218, 219, 222,
                                    #if defined(Q_OS_MAC)
                                                10
                                    #elif defined(Q_OS_WIN)
                                                50
                                    #else
                                                30
                                    #endif
                                                ));
            painter->fillRect(option.rect, alternateBrush);
        }

        QStyledItemDelegate::paint(painter, option, index);

        auto memoPage = index.data(Qt::UserRole).value<MemoTab*>();
        if (memoPage && !memoPage->isReadOnly())
        {
            static QImage modifiedMarker = makeMarker(":/icon/is_modified");
            static QImage editModeMarker = makeMarker(":/icon/edit_mode");
            painter->drawImage(option.rect.right() - 26, option.rect.top() + 4,
                memoPage->isModified() ? modifiedMarker : editModeMarker);
        }
    }
};
}

OpenTabsWidget::OpenTabsWidget() : QWidget()
{
    _tabsList = new QListWidget;
    _tabsList->setObjectName("tabs_list");
    connect(_tabsList, &QListWidget::currentItemChanged, this, &OpenTabsWidget::currentItemChanged);

    auto oldItemDelegate = _tabsList->itemDelegate();
    _tabsList->setItemDelegate(new OpenTabItemDelegate(this));
    if (oldItemDelegate) oldItemDelegate->deleteLater();

    Ori::Layouts::LayoutV({_tabsList}).setMargin(0).setSpacing(0).useFor(this);
}

void OpenTabsWidget::addOpenedTab(QWidget* tab)
{
    if (_tabsMap.contains(tab))
    {
        _tabsList->setCurrentItem(_tabsMap[tab]);
        return;
    }

    auto item = new QListWidgetItem(_tabsList);
    item->setText(tab->windowTitle());
    item->setIcon(tab->windowIcon());
    item->setData(Qt::UserRole, QVariant::fromValue(tab));


    connect(tab, &QWidget::destroyed, this, &OpenTabsWidget::tabDestroyed);
    connect(tab, &QWidget::windowTitleChanged, this, &OpenTabsWidget::tabTitleChanged);
    connect(tab, &QWidget::windowIconChanged, this, &OpenTabsWidget::tabIconChanged);

    auto memoPage = dynamic_cast<MemoTab*>(tab);
    if (memoPage)
    {
        updateTooltip(item, memoPage);
        connect(memoPage, &MemoTab::onReadOnly, this, &OpenTabsWidget::tabReadOnlyToggled);
        connect(memoPage, &MemoTab::onModified, this, &OpenTabsWidget::tabModified);
    }

    _tabsList->addItem(item);
    _tabsList->setCurrentItem(item);
    _tabsMap.insert(tab, item);
}

void OpenTabsWidget::tabDestroyed(QObject* obj)
{
    auto tab = reinterpret_cast<QWidget*>(obj); // <- qobject_cast returns null here
    if (_tabsMap.contains(tab))
    {
        auto item = _tabsMap[tab];
        _tabsMap.remove(tab);
        delete item;
    }
}

void OpenTabsWidget::currentItemChanged(QListWidgetItem *current, QListWidgetItem*)
{
    if (!current) return;
    auto tab = qvariant_cast<QWidget*>(current->data(Qt::UserRole));
    if (!tab)
    {
        qCritical() << "Invalid app state: no window is attached do item";
        return;
    }
    emit onActivateTab(tab);
}

void OpenTabsWidget::updateTooltip(QListWidgetItem *item, MemoTab *memoPage)
{
    auto memoItem = memoPage->memoItem();
    QString tooltip;
    QTextStream stream(&tooltip);
    stream << QStringLiteral("<p style='white-space:pre'>/%1/<b>%2</b>").arg(memoItem->path(), memoItem->title());
    if (!memoPage->isReadOnly())
    {
        stream << QStringLiteral("<br><span style='color:gray'>(");
        stream << tr("edit mode");
        if (memoPage->isModified())
            stream << tr(", modified");
        stream << QStringLiteral(")</span>");
    }
    item->setToolTip(tooltip);
}

void OpenTabsWidget::tabTitleChanged(const QString& title)
{
    auto tab = qobject_cast<QWidget*>(sender());
    if (!tab || !_tabsMap.contains(tab)) return;
    _tabsMap[tab]->setText(title);
}

void OpenTabsWidget::tabIconChanged(const QIcon& icon)
{
    auto tab = qobject_cast<QWidget*>(sender());
    if (!tab || !_tabsMap.contains(tab)) return;
    _tabsMap[tab]->setIcon(icon);
}

void OpenTabsWidget::tabReadOnlyToggled(bool)
{
    auto tab = qobject_cast<MemoTab*>(sender());
    if (!tab || !_tabsMap.contains(tab)) return;
    updateTooltip(_tabsMap[tab], tab);
    _tabsList->update();
}

void OpenTabsWidget::tabModified(bool)
{
    auto tab = qobject_cast<MemoTab*>(sender());
    if (!tab || !_tabsMap.contains(tab)) return;
    // QTextEdit::isModified() is not set yet when the signal is raised, have to defer
    QTimer::singleShot(0, [this, tab]{ updateTooltip(_tabsMap[tab], tab); });
    _tabsList->update();
}
