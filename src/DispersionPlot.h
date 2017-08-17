#ifndef DISPERSIONPLOT_H
#define DISPERSIONPLOT_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QTextEdit;
QT_END_NAMESPACE

class Catalog;
class GlassItem;


class PlotWindow
{

};

class DispersionPlot : public QWidget, public PlotWindow
{
    Q_OBJECT

public:
    explicit DispersionPlot(Catalog* catalog);
    ~DispersionPlot();

private:
    Catalog* _catalog;
    QTextEdit* _plotView;

    void glassRemoved(GlassItem* item);
};

#endif // DISPERSIONPLOT_H
