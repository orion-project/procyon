#include "OpenedPagesWidget.h"

#include "helpers/OriLayouts.h"
#include "pages/MemoPage.h"
#include "catalog/Catalog.h"

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

class OpenedPageItemDelegate : public QStyledItemDelegate
{
public:
    OpenedPageItemDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (index.row() % 2 == 0)
        {
            static QBrush alternateBrush(QColor(218, 219, 222, 30));
            painter->fillRect(option.rect, alternateBrush);
        }

        QStyledItemDelegate::paint(painter, option, index);

        auto memoPage = index.data(Qt::UserRole).value<MemoPage*>();
        if (memoPage && !memoPage->isReadOnly())
        {
            static QImage modifiedMarker = makeMarker(":/icon/is_modified");
            static QImage editModeMarker = makeMarker(":/icon/edit_mode");
            painter->drawImage(option.rect.right() - 28, option.rect.top() + 4,
                memoPage->isModified() ? modifiedMarker : editModeMarker);
        }
    }
};
}

OpenedPagesWidget::OpenedPagesWidget() : QWidget()
{
    _pagesList = new QListWidget;
    _pagesList->setObjectName("pages_list");
    connect(_pagesList, &QListWidget::currentItemChanged, this, &OpenedPagesWidget::currentItemChanged);

    auto oldItemDelegate = _pagesList->itemDelegate();
    _pagesList->setItemDelegate(new OpenedPageItemDelegate(this));
    if (oldItemDelegate) oldItemDelegate->deleteLater();

    Ori::Layouts::LayoutV({_pagesList}).setMargin(0).setSpacing(0).useFor(this);
}

void OpenedPagesWidget::addOpenedPage(QWidget* page)
{
    if (_pagesMap.contains(page))
    {
        _pagesList->setCurrentItem(_pagesMap[page]);
        return;
    }

    auto item = new QListWidgetItem(_pagesList);
    item->setText(page->windowTitle());
    item->setIcon(page->windowIcon());
    item->setData(Qt::UserRole, QVariant::fromValue(page));


    connect(page, &QWidget::destroyed, this, &OpenedPagesWidget::pageDestroyed);
    connect(page, &QWidget::windowTitleChanged, this, &OpenedPagesWidget::pageTitleChanged);
    connect(page, &QWidget::windowIconChanged, this, &OpenedPagesWidget::pageIconChanged);

    auto memoPage = dynamic_cast<MemoPage*>(page);
    if (memoPage)
    {
        updateTooltip(item, memoPage);
        connect(memoPage, &MemoPage::onReadOnly, this, &OpenedPagesWidget::pageReadOnlyToggled);
        connect(memoPage, &MemoPage::onModified, this, &OpenedPagesWidget::pageModified);
    }

    _pagesList->addItem(item);
    _pagesList->setCurrentItem(item);
    _pagesMap.insert(page, item);
}

void OpenedPagesWidget::pageDestroyed(QObject* obj)
{
    auto page = reinterpret_cast<QWidget*>(obj); // <- qobject_cast returns null here
    if (_pagesMap.contains(page))
    {
        auto item = _pagesMap[page];
        _pagesMap.remove(page);
        delete item;
    }
}

void OpenedPagesWidget::currentItemChanged(QListWidgetItem *current, QListWidgetItem*)
{
    if (!current) return;
    auto page = qvariant_cast<QWidget*>(current->data(Qt::UserRole));
    if (!page)
    {
        qCritical() << "Invalid app state: no window is attached do item";
        return;
    }
    emit onActivatePage(page);
}

void OpenedPagesWidget::updateTooltip(QListWidgetItem *item, MemoPage *memoPage)
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

void OpenedPagesWidget::pageTitleChanged(const QString& title)
{
    auto page = qobject_cast<QWidget*>(sender());
    if (!page || !_pagesMap.contains(page)) return;
    _pagesMap[page]->setText(title);
}

void OpenedPagesWidget::pageIconChanged(const QIcon& icon)
{
    auto page = qobject_cast<QWidget*>(sender());
    if (!page || !_pagesMap.contains(page)) return;
    _pagesMap[page]->setIcon(icon);
}

void OpenedPagesWidget::pageReadOnlyToggled(bool)
{
    auto page = qobject_cast<MemoPage*>(sender());
    if (!page || !_pagesMap.contains(page)) return;
    updateTooltip(_pagesMap[page], page);
    _pagesList->update();
}

void OpenedPagesWidget::pageModified(bool)
{
    auto page = qobject_cast<MemoPage*>(sender());
    if (!page || !_pagesMap.contains(page)) return;
    // QTextEdit::isModified() is not set yet when the signal is raised, have to defer
    QTimer::singleShot(0, [this, page]{ updateTooltip(_pagesMap[page], page); });
    _pagesList->update();
}
