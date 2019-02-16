#include "MemoTypeSelector.h"
#include "catalog/Catalog.h"

MemoType* MemoTypeSelector::selectType()
{
    // TODO init dialog
    return plainTextMemoType();

}

MemoTypeSelector::MemoTypeSelector(QWidget *parent) : QDialog(parent)
{

}
