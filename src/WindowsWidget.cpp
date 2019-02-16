#include "WindowsWidget.h"
#include "helpers/OriLayouts.h"

#include <QDebug>
#include <QListWidget>
#include <QMdiArea>
#include <QMdiSubWindow>

WindowsWidget::WindowsWidget(QMdiArea *parent) : QWidget(parent), _mdiArea(parent)
{
    connect(_mdiArea, &QMdiArea::subWindowActivated, this, &WindowsWidget::subWindowActivated);

    _windowsList = new QListWidget;
    // TODO alternate color looks too dark on some themes, e.g. Fusion on Windows,
    // should set alternate color via stylesheet and it should be only slightly different from base
    //_windowsList->setAlternatingRowColors(true);
    connect(_windowsList, &QListWidget::currentItemChanged, this, &WindowsWidget::currentItemChanged);

    Ori::Layouts::LayoutV({_windowsList}).setMargin(0).useFor(this);
}

void WindowsWidget::subWindowActivated(QMdiSubWindow* window)
{
    if (!window)
    {
        _windowsList->setCurrentItem(nullptr);
        return;
    }
    if (!_windowsMap.contains(window))
    {
        auto item = new QListWidgetItem(_windowsList);
        item->setText(window->windowTitle());
        item->setIcon(window->windowIcon());
        item->setData(Qt::UserRole, QVariant::fromValue(window));
        _windowsList->addItem(item);
        _windowsMap.insert(window, item);
        connect(window, &QMdiSubWindow::destroyed, this, &WindowsWidget::subWindowDestroyed);
        connect(window, &QMdiSubWindow::windowTitleChanged, this, &WindowsWidget::subWindowTitleChanged);
        connect(window, &QMdiSubWindow::windowIconChanged, this, &WindowsWidget::subWindowIconChanged);
    }
    _windowsList->setCurrentItem(_windowsMap[window]);
}

void WindowsWidget::subWindowDestroyed(QObject* obj)
{
    auto window = reinterpret_cast<QMdiSubWindow*>(obj); // <- qobject_cast returns null here
    if (_windowsMap.contains(window))
    {
        auto item = _windowsMap[window];
        _windowsMap.remove(window);
        delete item;
    }
}

void WindowsWidget::currentItemChanged(QListWidgetItem *current, QListWidgetItem*)
{
    if (!current) return;
    auto window = qvariant_cast<QMdiSubWindow*>(current->data(Qt::UserRole));
    if (!window)
    {
        qCritical() << "Invalid app state: no window is attached do item";
        return;
    }
    auto activeWindow = _mdiArea->activeSubWindow();
    // Do nothing if window is already active
    if (window == activeWindow) return;
    if (!window->isVisible()) window->show();
    if (window->windowState() | Qt::WindowMinimized) window->showNormal();
    auto isMaximized = activeWindow && activeWindow->windowState() & Qt::WindowMaximized;
    _mdiArea->setActiveSubWindow(window);
    if (isMaximized && !(window->windowState() & Qt::WindowMaximized))
        window->setWindowState(Qt::WindowMaximized);
}

void WindowsWidget::subWindowTitleChanged(const QString& title)
{
    auto window = qobject_cast<QMdiSubWindow*>(sender());
    if (!window || !_windowsMap.contains(window)) return;
    _windowsMap[window]->setText(title);
}

void WindowsWidget::subWindowIconChanged(const QIcon& icon)
{
    auto window = qobject_cast<QMdiSubWindow*>(sender());
    if (!window || !_windowsMap.contains(window)) return;
    _windowsMap[window]->setIcon(icon);
}
