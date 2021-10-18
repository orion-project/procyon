#ifndef POPUP_MESSAGE_H
#define POPUP_MESSAGE_H

#include <QWidget>

class PopupMessage : public QWidget
{
    Q_OBJECT

public:
    static void affirm(const QString& text, int duration = 2000);
    static void error(const QString& text, int duration = 2000);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    enum Mode {AFFIRM, ERROR};
    Mode _mode;
    explicit PopupMessage(Mode mode, const QString& text, int duration, QWidget *parent);
};

#endif // POPUP_MESSAGE_H
