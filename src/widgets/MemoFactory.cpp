#include "MemoFactory.h"

#include "core/MemoType.h"
#include "tabs/IssueMemoTab.h"

namespace MemoFactory {

MemoResult createMemo(Enot* enot, Folder* folder, MemoType *memoType)
{
    if (memoType == MemoType::issue())
    {
        IssueMemoTab::createIssue(enot, folder);
        return MemoResult::ok(nullptr);
    }

    return enot->createMemo(folder, memoType);
}

} // namespace MemoFactory

