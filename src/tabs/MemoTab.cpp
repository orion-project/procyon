#include "MemoTab.h"

#include "core/Enot.h"
#include "core/MemoType.h"

MemoTab::MemoTab(Enot *enot, Memo *memo) : QWidget(), _enot(enot), _memo(memo)
{
    setWindowIcon(_memo->type()->icon());
}
