#include "MemoTab.h"

#include "core/Db.h"
#include "core/MemoType.h"

MemoTab::MemoTab(Db *db, MemoItem *memoItem) : QWidget(), _db(db), _memoItem(memoItem)
{
    setWindowIcon(_memoItem->type()->icon());
}
