#include "OpenedPagesWidget.h"

#include "helpers/OriLayouts.h"

#include <QDebug>
#include <QListWidget>

OpenedPagesWidget::OpenedPagesWidget() : QWidget()
{
    _pagesList = new QListWidget;
    _pagesList->setObjectName("pages_list");
    connect(_pagesList, &QListWidget::currentItemChanged, this, &OpenedPagesWidget::currentItemChanged);

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
