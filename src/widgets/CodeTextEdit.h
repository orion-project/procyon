#ifndef CODE_TEXT_EDIT_H
#define CODE_TEXT_EDIT_H

#include <QPlainTextEdit>

class CodeTextEdit : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeTextEdit(const QString& highlighterName = QString(), QWidget *parent = nullptr);

    int lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

    void setLineHints(const QMap<int, QString>& hints);
    QString getLineHint(int y) const;

protected:
    void resizeEvent(QResizeEvent *e) override;

private:
    QWidget *_lineNumArea;
    QMap<int, QString> _lineHints;
    void updateLineNumberAreaWidth(int blockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();
    int findLineNumber(int y) const;
};

#endif // CODE_TEXT_EDIT_H
