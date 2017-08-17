#ifndef GLASSLISTWIDGET_H
#define GLASSLISTWIDGET_H

#include <QMap>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

class Catalog;
class GlassItem;

class GlassListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit GlassListWidget(QList<GlassItem*>* items);

    void populate();

private:
    QList<GlassItem*>* _items;
    QMap<GlassItem*, QWidget*> _itemPanels;
    QVBoxLayout* _layout;

    QWidget* makeGlassPanel(GlassItem* item) const;
};

#endif // GLASSLISTWIDGET_H
