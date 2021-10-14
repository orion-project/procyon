#ifndef CODE_TEXT_EDIT_H
#define CODE_TEXT_EDIT_H

#include <QPlainTextEdit>

class CodeTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeTextEdit(QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    QWidget *_lineNumberArea;
    void updateLineNumberAreaWidth(int blockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();
};

#endif // CODE_TEXT_EDIT_H
