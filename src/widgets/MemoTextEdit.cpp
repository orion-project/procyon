#include "MemoTextEdit.h"

#include "core/FileStore.h"
#include "../TextEditHelpers.h"

#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"

#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFormLayout>
#include <QImageWriter>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QTextBlock>
#include <QTimer>
#include <QToolTip>
#include <QRandomGenerator>

static const int __maxFileSize = 1*1024*1024; // TODO: make configurable

MemoTextEdit::MemoTextEdit(QWidget* parent) : QTextEdit(parent)
{
    setAcceptRichText(false);
    setWordWrapMode(QTextOption::NoWrap);
    setProperty("role", "memo_editor");
    setAcceptDrops(true);
}

void MemoTextEdit::setIsPlainText(bool on)
{
    _isPlainText = on;

    setAcceptDrops(!on);
}

bool MemoTextEdit::canInsertFromMimeData(const QMimeData* source) const
{
    if (_isPlainText)
        return QTextEdit::canInsertFromMimeData(source);

    return source->hasImage() || source->hasUrls() || QTextEdit::canInsertFromMimeData(source);
}

// Hyperlink made via syntax highlighter doesn't create some 'top level' anchor,
// so `anchorAt` returns nothing, we have to enumerate styles to find out a href.
QString MemoTextEdit::hyperlinkAt(const QPoint& pos) const
{
    return TextEditHelpers::hyperlinkAt(cursorForPosition(viewport()->mapFromParent(pos)));
}

void MemoTextEdit::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && e->modifiers().testFlag(Qt::ControlModifier))
        _clickedHref = hyperlinkAt(e->pos());

    // There is no selection -> move cursor to the point of click
    auto cursor = textCursor();
    if (e->button() == Qt::RightButton && cursor.anchor() == cursor.position())
        setTextCursor(cursorForPosition(viewport()->mapFromParent(e->pos())));

    QTextEdit::mousePressEvent(e);
}

void MemoTextEdit::mouseReleaseEvent(QMouseEvent *e)
{
    if (not _clickedHref.isEmpty())
    {
        QDesktopServices::openUrl(_clickedHref);
        _clickedHref.clear();
    }
    QTextEdit::mouseReleaseEvent(e);
}

bool MemoTextEdit::event(QEvent *event)
{
    if (event->type() != QEvent::ToolTip)
        return QTextEdit::event(event);

    auto helpEvent = dynamic_cast<QHelpEvent*>(event);
    if (not helpEvent) return false;

    auto href = hyperlinkAt(helpEvent->pos());
    if (not href.isEmpty())
    {
        auto tooltip = QStringLiteral("<p style='white-space:pre'>%1<p>%2")
                .arg(href, tr("<b>Ctrl + Click</b> to open"));
        QToolTip::showText(helpEvent->globalPos(), tooltip);
    }
    else QToolTip::hideText();

    event->accept();
    return true;
}

bool MemoTextEdit::wordWrap() const
{
    return wordWrapMode() != QTextOption::NoWrap;
}

void MemoTextEdit::setWordWrap(bool on)
{
    setWordWrapMode(on ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
}

bool MemoTextEdit::isModified() const
{
    return document()->isModified();
}

void MemoTextEdit::setModified(bool on)
{
    document()->setModified(on);
}

void MemoTextEdit::setReadOnly(bool on)
{
    QTextEdit::setReadOnly(on);
    Qt::TextInteractionFlags flags = Qt::LinksAccessibleByMouse |
        Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard;
    if (!on) flags |= Qt::TextEditable;
    setTextInteractionFlags(flags);
}

static QString makeFileName(const QString& ext)
{
    auto now = QDateTime::currentDateTime();
    auto date = now.date();
    auto time = now.time();
    return QStringLiteral("%1%2%3_%4%5%6_%7.%8")
            .arg(date.year(), 4, 10, QChar('0'))
            .arg(date.month(), 2, 10, QChar('0'))
            .arg(date.day(), 2, 10, QChar('0'))
            .arg(time.hour(), 2, 10, QChar('0'))
            .arg(time.minute(), 2, 10, QChar('0'))
            .arg(time.second(), 2, 10, QChar('0'))
            .arg(QString("%1").arg(QRandomGenerator::global()->generate(), 4, 16).right(4))
            .arg(ext);
}

void MemoTextEdit::pasteImage(const QImage& img)
{
    auto fi = Store::files()->attachedFile(makeFileName("png"));
    if (fi.exists())
    {
        Ori::Dlg::error(tr("File already exists: %1. Please try to add one more time, "
            "this should generate a different file name").arg(fi.absoluteFilePath()));
        return;
    }

    auto imgPreview = new QLabel;
    imgPreview->setPixmap(QPixmap::fromImage(img));
    auto w = Ori::Layouts::LayoutV({
        new QLabel(tr("<b>Target name:</b> %1").arg(fi.fileName())),
        new QLabel(tr("<b>Image size:</b> %1 × %2 px").arg(img.width()).arg(img.height())),
        imgPreview
    }).setMargin(0).makeWidget();

    if (Ori::Dlg::Dialog(w, true).withTitle(tr("Add Image")).withContentToButtonsSpacingFactor(2).exec())
    {
        if (!ensureFilesDir())
            return;
        QImageWriter writer(fi.absoluteFilePath());
        if (!writer.write(img))
        {
            Ori::Dlg::error(tr("Failed to save image %1: %2").arg(fi.absoluteFilePath(), writer.errorString()));
            return;
        }
        qDebug() << "Created" << fi.absoluteFilePath();
        _generatedFiles.append(fi.absoluteFilePath());
        textCursor().insertText(QString("![](%1)").arg(fi.fileName()));
    }
}

void MemoTextEdit::pasteFile(const QMimeData* source)
{
    auto urls = source->urls();
    if (urls.size() > 1)
    {
        Ori::Dlg::info(tr("Several files can not be added at once, please select only one file"));
        return;
    }
    auto url = urls.first();
    if (!url.isLocalFile())
    {
        Ori::Dlg::error(tr("Unsupported file source: %1").arg(url.toString()));
        return;
    }
    QFileInfo src(url.toLocalFile());
    if (!src.exists())
    {
        Ori::Dlg::error(tr("File not found: %1").arg(src.absoluteFilePath()));
        return;
    }
    if (src.size() > __maxFileSize)
    {
        Ori::Dlg::error(tr("File is too large (%1 bytes), maximum supported file size is %2 bytes").arg(src.size()).arg(__maxFileSize));
        return;
    }

    if (Store::files()->isSupportedImg(src.fileName()))
    {
        QImage img(src.absoluteFilePath());
        pasteImage(img);
        return;
    }

    auto dst = Store::files()->attachedFile(makeFileName(src.suffix()));
    if (dst.exists())
    {
        Ori::Dlg::error(tr("File already exists: %1. Please try to add one more time, "
            "this should generate a different file name").arg(dst.absoluteFilePath()));
        return;
    }

    auto w = new QWidget;
    auto form = new QFormLayout(w);
    form->setContentsMargins(0, 0, 0, 0);
    form->setLabelAlignment(Qt::AlignRight);
    form->setHorizontalSpacing(9);
    form->addRow(tr("<b>Copy from:</b>"), new QLabel(src.absoluteFilePath()));
    form->addRow(tr("<b>Target name:</b>"), new QLabel(dst.fileName()));
    form->addRow(tr("<b>Display as:</b>"), new QLabel(src.fileName()));
    form->addRow(tr("<b>File size:</b>"), new QLabel(tr("%1 bytes").arg(src.size())));

    if (Ori::Dlg::Dialog(w, true).withTitle(tr("Add File")).withContentToButtonsSpacingFactor(2).exec())
    {
        if (!ensureFilesDir())
            return;
        QFile srcFile(src.absoluteFilePath());
        if (!srcFile.open(QIODeviceBase::ReadOnly))
        {
            Ori::Dlg::error(tr("Unable to read source file %1: %2").arg(src.absoluteFilePath(), srcFile.errorString()));
            return;
        }
        if (!srcFile.copy(dst.absoluteFilePath()))
        {
            Ori::Dlg::error(tr("Failed to save file %1: %2").arg(dst.absoluteFilePath(), srcFile.errorString()));
            return;
        }
        qDebug() << "Created" << dst.absoluteFilePath();
        _generatedFiles.append(dst.absoluteFilePath());
        textCursor().insertText(QString("[%1](%2)").arg(src.fileName(), dst.fileName()));
    }
}

void MemoTextEdit::insertFromMimeData(const QMimeData* source)
{
    if (_isPlainText)
    {
        QTextEdit::insertFromMimeData(source);
        return;
    }

    if (source->hasImage())
    {
        auto img = qvariant_cast<QImage>(source->imageData());
        pasteImage(img);
        return;
    }
    if (source->hasUrls())
    {
        pasteFile(source);
        return;
    }

    QTextEdit::insertFromMimeData(source);
}

void MemoTextEdit::dropEvent(QDropEvent *event)
{
    if (_isPlainText)
    {
        QTextEdit::dropEvent(event);
        return;
    }

    insertFromMimeData(event->mimeData());
}

QString MemoTextEdit::cleanFiles()
{
    QStringList report;
    for (auto& fn : _generatedFiles)
    {
        QFile f(fn);
        if (!f.remove())
        {
            report.append(fn + ": " + f.errorString());
            qWarning() << "Deleting" << fn << f.errorString();
        }
        else qDebug() << "Deleted" << fn;
    }
    return report.join('\n');
}

bool MemoTextEdit::ensureFilesDir()
{
    auto fi = Store::files()->attachedFile({});
    if (fi.isDir() && fi.exists())
        return true;
    if (!QDir().mkdir(fi.absoluteFilePath()))
    {
        Ori::Dlg::error(tr("Failed to create files directory %1").arg(fi.absolutePath()));
        return false;
    }
    return true;
}
