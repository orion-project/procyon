#include "Catalog.h"
#include "Glass.h"
#include "GlassListWidget.h"
#include "helpers/OriLayouts.h"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Ori::Layouts;

GlassListWidget::GlassListWidget(QList<GlassItem*>* items) : QWidget(), _items(items)
{
    _layout = new QVBoxLayout;
    _layout->setMargin(0);
    _layout->setSpacing(0);
    setLayout(_layout);
}

void GlassListWidget::populate()
{
    for (GlassItem* item: *_items)
        if (!_itemPanels.contains(item))
        {
            auto panel = makeGlassPanel(item);
            _itemPanels.insert(item, panel);
            _layout->addWidget(panel);
        }
    for (GlassItem* item: _itemPanels.keys())
        if (!_items->contains(item))
        {
            delete _itemPanels[item];
            _itemPanels.remove(item);
        }
}

QWidget* GlassListWidget::makeGlassPanel(GlassItem* item) const
{
    auto icon = new QLabel;
    icon->setPixmap(item->glass()->formula()->icon().pixmap(16, 16));
    auto title = new QLabel(item->title());
    auto deleteButton = new QPushButton(QIcon(":/toolbar/delete"), QString());
    deleteButton->setFlat(true);
    deleteButton->setToolTip(tr("Exclude Material"));
    return LayoutH({icon, title, Stretch(), deleteButton}).setMargin(0).makeWidget();
}
