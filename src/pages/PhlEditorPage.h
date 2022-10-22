#ifndef PHL_EDITOR_PAGE_H
#define PHL_EDITOR_PAGE_H

#include "tools/OriHighlighter.h"

#include <QWidget>

namespace Ori {
namespace Widgets {
    class CodeEditor;
}}

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
    Ori::Widgets::CodeEditor *_editor;
    QPlainTextEdit *_sample;
    QSyntaxHighlighter *_highlight;

    void checkHighlighter();
    void saveHighlighter();
};

#endif // PHL_EDITOR_PAGE_H
