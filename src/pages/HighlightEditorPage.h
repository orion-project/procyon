#ifndef HIGHLIGHT_EDITOR_PAGE_H
#define HIGHLIGHT_EDITOR_PAGE_H

#include "../highlighter/OriHighlighter.h"

#include <QWidget>

class HighlightEditorPage : public QWidget
{
    Q_OBJECT
public:
    explicit HighlightEditorPage(const QSharedPointer<Ori::Highlighter::Spec>& spec);

    QSharedPointer<Ori::Highlighter::Spec> spec;
};

#endif // HIGHLIGHT_EDITOR_PAGE_H
