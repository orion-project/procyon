#ifndef MEMO_PROPS_PANEL_H
#define MEMO_PROPS_PANEL_H

#include <QFrame>

class Enot;
class MemoPropWidget;

class MemoPropsPanel : public QFrame
{
    Q_OBJECT

public:
    MemoPropsPanel(Enot* enot);

    void addPropViaDlg();
    void addProp(const QString& name, const QString& value);

    void setReadOnly(bool on);

    void apply();

    bool hasValues() const { return _hasValues; }
    QHash<QString, QString> values() const;

private:
    Enot *_enot;
    QMenu *_menu;
    bool _isReadonly = true;
    bool _hasValues = false;
    QString _activeProp;
    QAction *_actionAddValue, *_actionDeleteProp;

    struct ValueView
    {
        QString value;
        QWidget *contentWidget;
        QLabel *readonlyLabel;
        QLabel *editableLabel = nullptr;
        bool transient = false;
    };

    QHash<QString, MemoPropWidget*> _valueViews;
    QList<MemoPropWidget*> _removedProps;

    void switchToEditable();
    void switchToReadonly();

    void updateValuesMenu();
    void addNewValue();
    void deleteProp();
};

#endif // MEMO_PROPS_PANEL_H
