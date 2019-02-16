#include "Memo.h"

Memo::~Memo()
{
}

PlainTextMemo::PlainTextMemo(MemoType* type) : Memo(type)
{
}

WikiTextMemo::WikiTextMemo(MemoType* type) : Memo(type)
{
}

RichTextMemo::RichTextMemo(MemoType* type) : Memo(type)
{
}
