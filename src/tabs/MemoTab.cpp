#include "MemoTab.h"

#include "core/Enot.h"
#include "core/MemoType.h"

MemoTab::MemoTab(Enot *enot, MemoItem *memoItem) : QWidget(), _enot(enot), _memoItem(memoItem)
{
    setWindowIcon(_memoItem->type()->icon());
}
