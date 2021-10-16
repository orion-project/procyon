#include "PopupMessage.h"

#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QPropertyAnimation>
#include <QTimer>

void PopupMessage::showAffirm(const QString& text)
{
    (new PopupMessage(text, qApp->activeWindow()))->show();
}

PopupMessage::PopupMessage(const QString& text, QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    auto layout = new QHBoxLayout(this);
    layout->setSpacing(20);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->addWidget(new QLabel(text));

    auto shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(20);
    shadow->setOffset(2);
    setGraphicsEffect(shadow);

    adjustSize();
    auto sz = size();
    auto psz = parent->size();
    int x = (psz.width() - sz.width())/2;
    int y = (psz.height() - sz.height())/2;
    move(x, y);

    QTimer::singleShot(1000, this, [this](){
        auto opacity = new QGraphicsOpacityEffect();
        setGraphicsEffect(opacity);
        auto fadeout = new QPropertyAnimation(opacity, "opacity");
        fadeout->setDuration(1000);
        fadeout->setStartValue(1);
        fadeout->setEndValue(0);
        fadeout->setEasingCurve(QEasingCurve::OutBack);
        fadeout->start(QPropertyAnimation::DeleteWhenStopped);
        connect(fadeout, &QPropertyAnimation::finished, this, &PopupMessage::close);
    });
}

void PopupMessage::mouseReleaseEvent(QMouseEvent*)
{
    close();
}

void PopupMessage::paintEvent(QPaintEvent *event)
{
    QPainter p(this);
    p.setClipRect(event->rect());
    p.setPen(Qt::gray);
    p.setBrush(QColor("PaleGreen"));
    p.setRenderHint(QPainter::Antialiasing, true);
    p.drawRoundedRect(0, 0, width(), height(), 5, 5);
    QWidget::paintEvent(event);
}
