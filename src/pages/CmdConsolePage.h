#ifndef TOOL_CONSOLE_PAGE_H
#define TOOL_CONSOLE_PAGE_H

#include <QWidget>

namespace CmdConsoleImpl {
    struct CmdConsole;
}

class Catalog;

class CmdConsolePage : public QWidget
{
    Q_OBJECT
public:
    explicit CmdConsolePage(Catalog* catalog);
    void setCatalog(Catalog* catalog);
private:
    QSharedPointer<CmdConsoleImpl::CmdConsole> _impl;
};

#endif // TOOL_CONSOLE_PAGE_H
