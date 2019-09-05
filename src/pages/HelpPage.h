#ifndef HELPPAGE_H
#define HELPPAGE_H

#include <QWidget>

class HelpPage : public QWidget
{
    Q_OBJECT

public:
    explicit HelpPage(QWidget *parent = nullptr);

    static void showAbout();
};

#endif // HELPPAGE_H
