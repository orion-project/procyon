#ifndef OPENED_PAGES_WIDGET_H
#define OPENED_PAGES_WIDGET_H

#include <QWidget>
#include <QMap>

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
QT_END_NAMESPACE

class OpenedPagesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OpenedPagesWidget();

    void addOpenedPage(QWidget*);

signals:
    void onActivatePage(QWidget* page);

private:
    QListWidget* _pagesList;
    QMap<QWidget*, QListWidgetItem*> _pagesMap;

    void pageDestroyed(QObject*);
    void pageTitleChanged(const QString& title);
    void pageIconChanged(const QIcon& icon);
    void currentItemChanged(QListWidgetItem*, QListWidgetItem*);
};

#endif // OPENED_PAGES_WIDGET_H
