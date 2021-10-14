#include "CodeTextEdit.h"

#include <QPainter>
#include <QTextBlock>

namespace {

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeTextEdit *editor) : QWidget(editor), _editor(editor)
    {}

    QSize sizeHint() const override
    {
        return QSize(_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        _editor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeTextEdit *_editor;
};

struct EditorStyle
{
    QBrush currentLineColor = QColor("steelBlue").lighter(220);
    QPen lineNumTextPen = QPen(Qt::gray);
    QColor lineNumBorderColor = QColor("silver");
    int lineNumRightMargin = 4;
    int lineNumLeftMargin = 6;
};

const EditorStyle& editorStyle()
{
    static EditorStyle style;
    return style;
}

} // namespace

CodeTextEdit::CodeTextEdit(QWidget *parent) : QPlainTextEdit(parent)
{
    setProperty("role", "memo_editor");
    setObjectName("code_editor");
    setWordWrapMode(QTextOption::NoWrap);
    /*
        auto f = editor->font();
    #if defined(Q_OS_WIN)
        f.setFamily("Courier New");
        f.setPointSize(11);
    #elif defined(Q_OS_MAC)
        f.setFamily("Monaco");
        f.setPointSize(13);
    #else
        f.setFamily("monospace");
        f.setPointSize(11);
    #endif
        editor->setFont(f);
    */

    _lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeTextEdit::blockCountChanged, this, &CodeTextEdit::updateLineNumberAreaWidth);
    connect(this, &CodeTextEdit::updateRequest, this, &CodeTextEdit::updateLineNumberArea);
    connect(this, &CodeTextEdit::cursorPositionChanged, this, &CodeTextEdit::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

void CodeTextEdit::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    _lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

int CodeTextEdit::lineNumberAreaWidth() const
{
    int digits = 1;
    // in a plain text edit each line will consist of one QTextBlock
    int max = qMax(1, blockCount());
    while (max >= 10)
    {
        max /= 10;
        digits++;
    }
    const auto& style = editorStyle();
    return style.lineNumLeftMargin + style.lineNumRightMargin +
            fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeTextEdit::updateLineNumberAreaWidth(int blockCount)
{
    Q_UNUSED(blockCount)
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeTextEdit::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        _lineNumberArea->scroll(0, dy);
    else
        _lineNumberArea->update(0, rect.y(), _lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeTextEdit::highlightCurrentLine()
{
    QTextEdit::ExtraSelection selection;
    selection.format.setBackground(editorStyle().currentLineColor);
    selection.format.setProperty(QTextFormat::FullWidthSelection, true);
    selection.cursor = textCursor();
    selection.cursor.clearSelection();
    setExtraSelections({selection});
}

void CodeTextEdit::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    const auto& style = editorStyle();

    QPainter painter(_lineNumberArea);
    painter.fillRect(event->rect(), Qt::white);
    const auto& r = event->rect();
    int lineNumW = _lineNumberArea->width();
    painter.setPen(style.lineNumBorderColor);
    painter.drawLine(lineNumW-1, r.top(), lineNumW-1, r.bottom());

    QTextBlock block = firstVisibleBlock();
    int lineNum = block.blockNumber() + 1;
    int lineTop = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int lineH = qRound(blockBoundingRect(block).height());
    int lineBot = lineTop + lineH;
    while (block.isValid() && lineTop <= r.bottom())
    {
        if (block.isVisible() && lineBot >= r.top())
        {
            painter.setPen(style.lineNumTextPen);
            painter.drawText(0, lineTop, lineNumW - style.lineNumRightMargin, lineH,
                Qt::AlignRight|Qt::AlignVCenter, QString::number(lineNum));
        }
        block = block.next();
        lineTop = lineBot;
        lineH = qRound(blockBoundingRect(block).height());
        lineBot = lineTop + lineH;
        lineNum++;
    }
}
