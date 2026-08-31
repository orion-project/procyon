#ifndef MEMO_TEXT_EDIT_H
#define MEMO_TEXT_EDIT_H

#include <QTextEdit>

class MemoTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit MemoTextEdit(QWidget* parent = nullptr);

    bool wordWrap() const;
    void setWordWrap(bool on);

    bool isModified() const;
    void setModified(bool on);

    void setReadOnly(bool on);

    QString cleanFiles();

protected:
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    bool event(QEvent *event) override;
    bool canInsertFromMimeData(const QMimeData* source) const override;
    void insertFromMimeData(const QMimeData* source) override;
    void dropEvent(QDropEvent *event) override;

private:
    QString _clickedHref;
    QStringList _generatedFiles;

    QString hyperlinkAt(const QPoint& pos) const;

    void pasteImage(const QImage& img);
    void pasteFile(const QMimeData* source);
    bool ensureFilesDir();
};

#endif // MEMO_TEXT_EDIT_H
