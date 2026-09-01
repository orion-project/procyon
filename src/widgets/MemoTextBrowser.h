#ifndef MEMO_TEXT_BROWSER_H
#define MEMO_TEXT_BROWSER_H

#include <QTextBrowser>

class MemoTextBrowser : public QTextBrowser
{
    Q_OBJECT

public:
    explicit MemoTextBrowser(QWidget *parent = nullptr);

    void setText(const QString& text);

signals:
    void memoOpenRequested(int id);

protected:
    bool event(QEvent *event) override;

private:
    void linkClicked(const QUrl& url);
};

#endif // MEMO_TEXT_BROWSER_H
