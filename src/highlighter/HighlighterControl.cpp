#include "HighlighterControl.h"
#include "HighlighterManager.h"

#include <QAction>
#include <QMenu>

HighlighterControl::HighlighterControl(QObject *parent) : QObject(parent)
{
    _actionGroup = new QActionGroup(parent);
    _actionGroup->setExclusive(true);
    connect(_actionGroup, &QActionGroup::triggered, this, &HighlighterControl::actionGroupTriggered);

    auto actionNone = new QAction(tr("None"), this);
    actionNone->setCheckable(true);
    _actionGroup->addAction(actionNone);

    for (auto h : HighlighterManager::instance().highlighters())
    {
        auto actionDict = new QAction(h.title, this);
        actionDict->setCheckable(true);
        actionDict->setData(h.name);
        _actionGroup->addAction(actionDict);
    }
}

QMenu* HighlighterControl::makeMenu(QWidget* parent)
{
    if (!_actionGroup) return nullptr;

    auto menu = new QMenu(tr("Highlighter"), parent);
    menu->addActions(_actionGroup->actions());
    return menu;
}

void HighlighterControl::showCurrent(const QString& name)
{
    if (!_actionGroup) return;

    for (auto action : _actionGroup->actions())
        if (action->data().toString() == name)
        {
            action->setChecked(true);
            break;
        }
}

void HighlighterControl::setEnabled(bool on)
{
    if (_actionGroup) _actionGroup->setEnabled(on);
}

void HighlighterControl::actionGroupTriggered(QAction* action)
{
    emit selected(action->data().toString());
}

