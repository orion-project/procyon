#ifndef MEMO_PROPS_PANEL_H
#define MEMO_PROPS_PANEL_H

#include <QFrame>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

class Enot;
class MemoPropWidget;

class MemoPropsPanel : public QFrame
{
    Q_OBJECT

public:
    MemoPropsPanel(Enot* enot, std::initializer_list<QWidget*> persistentWidgets = {});

    void addPropViaDlg();
    void addProp(const QString& name, const QString& value);

    void setReadOnly(bool on);

    void apply();

    bool hasValues() const { return _hasValues; }
    QHash<QString, QString> values() const;
    void setValues(const QHash<QString, QString>& values);
    bool isModified() const;

    bool hideWhenEmpty = true;

private:
    Enot *_enot;
    QMenu *_menu;
    bool _isReadonly = true;
    bool _hasValues = false;
    QString _activeProp;
    QAction *_actionAddValue, *_actionDeleteProp;
    QLayout *_propsLayout;
    QHash<QString, MemoPropWidget*> _valueViews;
    QList<MemoPropWidget*> _removedProps;
    QHash<QString, QString> _originalValues;

    void switchToEditable();
    void switchToReadonly();

    void updateValuesMenu();
    void addNewValue();
    void deleteProp();
};

#endif // MEMO_PROPS_PANEL_H
