#ifndef TEXT_EDIT_SPELLCHECK_H
#define TEXT_EDIT_SPELLCHECK_H

#include <QPointer>
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
    explicit TextEditSpellcheck(QTextEdit* editor, const QString &lang, QObject *parent = nullptr);
    ~TextEditSpellcheck();
    void clearErrorMarks();
    void spellcheckAll();

private:
    QPointer<QTextEdit> _editor;
    Spellchecker* _spellchecker = nullptr;
    QTimer* _timer = nullptr;
    int _changesStart = -1;
    int _changesStop = -1;
    int _spellcheckStart = -1;
    int _spellcheckStop = -1;
    QList<QTextEdit::ExtraSelection> _errorMarks;
    bool _changesLocked = false;

    void spellcheck();
    QTextCursor spellingAt(const QPoint& pos) const;
    void contextMenuRequested(const QPoint &pos);
    void addSpellcheckActions(QMenu* menu, QTextCursor &cursor);
    void removeErrorMark(const QTextCursor& cursor);
    void documentChanged(int position, int charsRemoved, int charsAdded);
    void spellcheckChanges();
};

#endif // TEXT_EDIT_SPELLCHECK_H
