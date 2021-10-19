#ifndef PHL_EDITOR_PAGE_H
#define PHL_EDITOR_PAGE_H

#include "../highlighter/OriHighlighter.h"

#include <QWidget>

class CodeTextEdit;

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

class PhlEditorPage : public QWidget
{
    Q_OBJECT

public:
    explicit PhlEditorPage(const QSharedPointer<Ori::Highlighter::Spec>& spec);

    QSharedPointer<Ori::Highlighter::Spec> spec;

private:
    CodeTextEdit *_editor;
    QPlainTextEdit *_sample;
    QSyntaxHighlighter *_highlight;

    void checkHighlighter();
    void saveHighlighter();
};

#endif // PHL_EDITOR_PAGE_H
