#ifndef WINDOWSWIDGET_H
#define WINDOWSWIDGET_H

#include <QWidget>
#include <QMap>

QT_BEGIN_NAMESPACE
class QListWidget;
class QListWidgetItem;
class QMdiArea;
class QMdiSubWindow;
QT_END_NAMESPACE

class WindowsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit WindowsWidget(QMdiArea *parent);

private:
    QMdiArea* _mdiArea;
    QListWidget* _windowsList;
    QMap<QMdiSubWindow*, QListWidgetItem*> _windowsMap;

    void subWindowActivated(QMdiSubWindow*);
    void subWindowDestroyed(QObject*);
    void currentItemChanged(QListWidgetItem*, QListWidgetItem*);
};

#endif // WINDOWSWIDGET_H
