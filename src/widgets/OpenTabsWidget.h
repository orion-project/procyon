#ifndef OPENED_TABS_WIDGET_H
#define OPENED_TABS_WIDGET_H

#include <QWidget>
#include <QMap>

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
QT_END_NAMESPACE

class MemoTab;

class OpenTabsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OpenTabsWidget();

    void addOpenedTab(QWidget*);

signals:
    void onActivateTab(QWidget* tab);

private:
    QListWidget* _tabsList;
    QMap<QWidget*, QListWidgetItem*> _tabsMap;

    void tabDestroyed(QObject*);
    void tabTitleChanged(const QString& title);
    void tabIconChanged(const QIcon& icon);
    void currentItemChanged(QListWidgetItem*, QListWidgetItem*);
    void tabReadOnlyToggled(bool);
    void tabModified(bool);
    void updateTooltip(QListWidgetItem*, MemoTab*);
};

#endif // OPENED_TABS_WIDGET_H
