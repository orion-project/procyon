#include "MemoTab.h"

#include "db/Db.h"
#include "db/MemoType.h"

MemoTab::MemoTab(Db *db, MemoItem *memoItem) : QWidget(), _db(db), _memoItem(memoItem)
{
    setWindowIcon(_memoItem->type()->icon());
}
