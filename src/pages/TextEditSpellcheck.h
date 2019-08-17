#ifndef TEXT_EDIT_SPELLCHECK_H
#define TEXT_EDIT_SPELLCHECK_H

#include <QTextEdit>

class Spellchecker;

QT_BEGIN_NAMESPACE
class QAction;
class QTimer;
QT_END_NAMESPACE

class TextEditSpellcheck : public QObject
{
    Q_OBJECT

public:
    explicit TextEditSpellcheck(QTextEdit* editor, QObject *parent = nullptr);
    Spellchecker* spellchecker() const { return _spellchecker; }
    void setSpellchecker(Spellchecker* checker);
    void clearErrorMarks();
    void spellcheckAll();

private:
    QTextEdit* _editor;
    Spellchecker* _spellchecker = nullptr;
    QTimer* _timer = nullptr;
    int _startPos = -1;
    int _stopPos = -1;
    QList<QTextEdit::ExtraSelection> _spellErrorMarks;
    bool _changesLocked = false;

    void spellcheckChanges();
    void spellcheck();
    QTextCursor spellingAt(const QPoint& pos) const;
    void contextMenuRequested(const QPoint &pos);
    void addSpellcheckActions(QMenu* menu, QTextCursor &cursor);
    void removeErrorMark(const QTextCursor& cursor);
    void documentContentChanged(int position, int charsRemoved, int charsAdded);
};

#endif // TEXT_EDIT_SPELLCHECK_H
