#ifndef HELP_PAGE_H
#define HELP_PAGE_H

#include <QWidget>

class HelpPage : public QWidget
{
    Q_OBJECT

public:
    explicit HelpPage(QWidget *parent = nullptr);

    static void showAbout();
    static void visitHomePage();
    static void sendBugReport();
};

#endif // HELP_PAGE_H
