#ifndef TOOL_CONSOLE_TAB_H
#define TOOL_CONSOLE_TAB_H

#include <QWidget>

namespace CmdConsoleImpl {
    struct CmdConsole;
}

class Db;

class CmdConsoleTab : public QWidget
{
    Q_OBJECT
public:
    explicit CmdConsoleTab(Db* catalog);
    void setCatalog(Db* catalog);
private:
    QSharedPointer<CmdConsoleImpl::CmdConsole> _impl;
};

#endif // TOOL_CONSOLE_TAB_H
