#ifndef HELP_TAB_H
#define HELP_TAB_H

#include <QWidget>

class HelpTab : public QWidget
{
    Q_OBJECT

public:
    explicit HelpTab(QWidget *parent = nullptr);

    static void showAbout();
    static void visitHomePage();
    static void sendBugReport();
};

#endif // HELP_TAB_H
