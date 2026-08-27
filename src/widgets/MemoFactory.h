#ifndef MEMO_FACTORY_H
#define MEMO_FACTORY_H

#include "core/Enot.h"

namespace MemoFactory
{
MemoResult createMemo(Enot* enot, Folder* folder, MemoType *memoType);
}

#endif // MEMO_FACTORY_H
