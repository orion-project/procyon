#ifndef POPUP_MESSAGE_H
#define POPUP_MESSAGE_H

#include <QWidget>

class PopupMessage : public QWidget
{
    Q_OBJECT

public:
    static void showAffirm(const QString& text);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseReleaseEvent(QMouseEvent*) override;

private:
    explicit PopupMessage(const QString& text, QWidget *parent = nullptr);
};

#endif // POPUP_MESSAGE_H
