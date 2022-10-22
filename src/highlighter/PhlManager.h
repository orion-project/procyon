#ifndef PHL_MANAGER_H
#define PHL_MANAGER_H

#include <QWidget>

#include "orion/tools/OriHighlighter.h"

QT_BEGIN_NAMESPACE
class QActionGroup;
class QListWidget;
class QMenu;
class QPlainTextEdit;
class QPushButton;
QT_END_NAMESPACE

namespace Phl {

QSharedPointer<Ori::Highlighter::Spec> getSpec(const QString& name);
QPair<bool, bool> checkDuplicates(const Ori::Highlighter::Meta& meta);
QSyntaxHighlighter* createHighlighter(QPlainTextEdit* editor, const QString& name);

class Control : public QObject
{
    Q_OBJECT

public:
    explicit Control(QMenu* menu, QObject *parent = nullptr);

    void showManager();
    void showCurrent(const QString& name);
    void setEnabled(bool on);

    void loadMetas();

signals:
    void selected(const QString& highlighter);
    void editorRequested(const QSharedPointer<Ori::Highlighter::Spec>& spec);

private:
    QMenu* _menu;
    QActionGroup* _actionGroup = nullptr;
    class ManagerDlg* _managerDlg = nullptr;

    void actionGroupTriggered(QAction* action);
    void makeMenu();
};


class ManagerDlg : public QWidget
{
private:
    QListWidget *_specList;
    Control *_parent;

    ManagerDlg(Control *parent);
    friend class Control;

    QString selectedSpecName() const;

    void editHighlighter();
    void createHighlighterEmpty();
    void createHighlighterCopy();
    void deleteHighlighter();
    void newHighlighterWithBase(const Ori::Highlighter::Meta& meta);
};

} // namespace Phl

#endif // PHL_MANAGER_H
