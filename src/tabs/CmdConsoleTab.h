#ifndef TOOL_CONSOLE_TAB_H
#define TOOL_CONSOLE_TAB_H

#include <QWidget>

namespace CmdConsoleImpl {
    struct CmdConsole;
}

class Catalog;

class CmdConsoleTab : public QWidget
{
    Q_OBJECT
public:
    explicit CmdConsoleTab(Catalog* catalog);
    void setCatalog(Catalog* catalog);
private:
    QSharedPointer<CmdConsoleImpl::CmdConsole> _impl;
};

#endif // TOOL_CONSOLE_TAB_H
