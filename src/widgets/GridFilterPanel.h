#ifndef GRID_FILTER_PANEL_H
#define GRID_FILTER_PANEL_H

#include <QFrame>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

class Enot;
class FilterPropWidget;

class GridFilterPanel : public QFrame
{
    Q_OBJECT

public:

    GridFilterPanel(Enot *enot);

    QString titleFilter() const;
    void setTitleFilter(const QString& value);
    QList<QPair<QString, QString>> propFilters() const;
    void setPropFilters(const QList<QPair<QString, QString>>& propFilter);

    void tryApplyFilters();
    void focusTitleFilter();

signals:
    void filterChanged();

private:
    Enot *_enot;
    QLineEdit *_titleFilter;
    QList<FilterPropWidget*> _propFilters;
};

#endif // GRID_FILTER_PANEL_H
