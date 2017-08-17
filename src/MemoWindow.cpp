#include "Appearance.h"
#include "Catalog.h"
#include "Memo.h"
#include "MemoWindow.h"
#include "helpers/OriWidgets.h"
#include "helpers/OriLayouts.h"

#include <QIcon>
#include <QDebug>
#include <QTextEdit>

using namespace Ori::Layouts;

//------------------------------------------------------------------------------

MemoWindow::MemoWindow(Catalog *catalog, MemoItem *memo) : QWidget()
{
    setWindowIcon(QIcon(":/icon/plot")); // TODO icon

    connect(catalog, &Catalog::memoRemoved, this, &MemoWindow::memoRemoved);

    _memo = memo;
    _memoEditor = new QTextEdit;
    Ori::Gui::setFontMonospace(_memoEditor);

    // TODO load memo data

    Ori::Layouts::LayoutV({_memoEditor}).setMargin(0).setSpacing(0).useFor(this);
}

MemoWindow::~MemoWindow()
{
}

void MemoWindow::memoRemoved(MemoItem* item)
{
    if (item == _memo)
    {
        // TODO close window
    }
}



