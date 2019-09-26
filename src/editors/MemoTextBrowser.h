#ifndef MEMO_TEXT_BROWSER_H
#define MEMO_TEXT_BROWSER_H

#include <QTextBrowser>

class MemoTextBrowser : public QTextBrowser
{
    Q_OBJECT

public:
    explicit MemoTextBrowser(QWidget *parent = nullptr);

protected:
    bool event(QEvent *event) override;
};

#endif // MEMO_TEXT_BROWSER_H
