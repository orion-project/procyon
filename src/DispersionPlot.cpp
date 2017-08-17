#include "Appearance.h"
#include "Catalog.h"
#include "Glass.h"
#include "DispersionPlot.h"
#include "helpers/OriWidgets.h"
#include "helpers/OriLayouts.h"

#include <QIcon>
#include <QDebug>
#include <QTextEdit>

using namespace Ori::Layouts;

//------------------------------------------------------------------------------

DispersionPlot::DispersionPlot(Catalog* catalog) : QWidget(), _catalog(catalog)
{
    setWindowIcon(QIcon(":/icon/plot")); // TODO

    connect(_catalog, &Catalog::glassRemoved, this, &DispersionPlot::glassRemoved);

    _plotView = new QTextEdit;
    Ori::Gui::setFontMonospace(_plotView);

    // TODO load note

    Ori::Layouts::LayoutV({_plotView}).setMargin(0).setSpacing(0).useFor(this);
}

DispersionPlot::~DispersionPlot()
{
}

void DispersionPlot::glassRemoved(GlassItem* item)
{
    // TODO close window
}



