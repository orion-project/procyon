#include "MemoTab.h"

#include "core/Enot.h"
#include "core/MemoType.h"

MemoTab::MemoTab(Enot *db, MemoItem *memoItem) : QWidget(), _db(db), _memoItem(memoItem)
{
    setWindowIcon(_memoItem->type()->icon());
}
