#ifndef HIGHLIGHTER_CONTROL_H
#define HIGHLIGHTER_CONTROL_H

#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QMenu;
QT_END_NAMESPACE

class HighlighterControl : public QObject
{
    Q_OBJECT

public:
    explicit HighlighterControl(QObject *parent = nullptr);

    QMenu* makeMenu(QWidget* parent = nullptr);

    void showCurrent(const QString& name);
    void setEnabled(bool on);

signals:
    void selected(const QString& highlighter);

private:
    QActionGroup* _actionGroup = nullptr;

    void actionGroupTriggered(QAction* action);
};

#endif // HIGHLIGHTER_CONTROL_H
