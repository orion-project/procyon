#include "CodeTextEdit.h"
#include "../highlighter/OriHighlighter.h"

#include <QPainter>
#include <QTextBlock>
#include <QToolTip>

namespace {

class LineNumberArea : public QWidget
{
public:
    LineNumberArea(CodeTextEdit *editor) : QWidget(editor), _editor(editor)
    {}

    QSize sizeHint() const override {
        return QSize(_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        _editor->lineNumberAreaPaintEvent(event);
    }

    bool event(QEvent *event) override
    {
        if (event->type() != QEvent::ToolTip)
            return QWidget::event(event);

        auto helpEvent = dynamic_cast<QHelpEvent*>(event);
        if (!helpEvent) return false;

        auto hint = _editor->getLineHint(helpEvent->pos().y());
        if (hint.isEmpty())
            QToolTip::hideText();
        else
            QToolTip::showText(helpEvent->globalPos(), hint);
        event->accept();
        return true;
    }

private:
    CodeTextEdit *_editor;
};

struct EditorStyle
{
    QBrush currentLineColor = QColor("steelBlue").lighter(220);
    QPen lineNumTextColor = QPen(Qt::gray);
    QPen lineNumTextColorErr = QPen(Qt::white);
    QPen lineNumBorderColor = QColor("silver");
    QBrush lineNumBackColorErr = QColor(Qt::red).lighter(130);
    int lineNumRightMargin = 4;
    int lineNumLeftMargin = 6;
};

const EditorStyle& editorStyle()
{
    static EditorStyle style;
    return style;
}

} // namespace

CodeTextEdit::CodeTextEdit(const QString &highlighterName, QWidget *parent) : QPlainTextEdit(parent)
{
    setProperty("role", "memo_editor");
    setObjectName("code_editor");
    setWordWrapMode(QTextOption::NoWrap);

    if (!highlighterName.isEmpty())
        Ori::Highlighter::createHighlighter(this, highlighterName);

    _lineNumArea = new LineNumberArea(this);

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
    _lineNumArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
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
        _lineNumArea->scroll(0, dy);
    else
        _lineNumArea->update(0, rect.y(), _lineNumArea->width(), rect.height());

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

    QPainter painter(_lineNumArea);
    painter.fillRect(event->rect(), Qt::white);
    const auto& r = event->rect();
    int lineNumW = _lineNumArea->width();
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
            if (_lineHints.contains(lineNum))
            {
                painter.setPen(style.lineNumTextColorErr);
                painter.fillRect(0, lineTop, lineNumW, lineH, style.lineNumBackColorErr);
            }
            else
                painter.setPen(style.lineNumTextColor);
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

void CodeTextEdit::setLineHints(const QMap<int, QString>& hints)
{
    _lineHints = hints;
    update();
}

int CodeTextEdit::findLineNumber(int y) const
{
    QTextBlock block = firstVisibleBlock();
    int lineNum = block.blockNumber() + 1;
    int lineTop = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int lineBot = lineTop + qRound(blockBoundingRect(block).height());
    while (block.isValid())
    {
        if (y >= lineTop && y < lineBot)
            return lineNum;
        block = block.next();
        lineTop = lineBot;
        lineBot = lineTop + qRound(blockBoundingRect(block).height());
        lineNum++;
    }
    return 0;
}

QString CodeTextEdit::getLineHint(int y) const
{
    return _lineHints.isEmpty() ? QString() : _lineHints[findLineNumber(y)];
}
