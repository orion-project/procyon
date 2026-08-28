#include "TextEditHelpers.h"

#include "helpers/OriDialogs.h"

#include <QApplication>
#include <QFontDialog>
#include <QMessageBox>
#include <QTextBlock>
#include <QStyle>
#include <QPrinter>

//------------------------------------------------------------------------------
//                                  TextFormat
//------------------------------------------------------------------------------

QTextCharFormat TextFormat::get() const
{
    QTextCharFormat f;
    if (!_fontFamily.isEmpty())
        f.setFontFamilies({_fontFamily});
    if (!_colorName.isEmpty())
        f.setForeground(QColor(_colorName));
    if (_bold)
        f.setFontWeight(QFont::Bold);
    f.setFontItalic(_italic);
    f.setFontUnderline(_underline);
    f.setAnchor(_anchor);
    f.setFontStrikeOut(_strikeOut);
    if (_spellError)
    {
        f.setUnderlineColor("red");
        f.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    }
    if (!_backColorName.isEmpty())
        f.setBackground(QColor(_backColorName));
    return f;
}

//------------------------------------------------------------------------------
//                                TextEditHelpers
//------------------------------------------------------------------------------

namespace TextEditHelpers
{

// Hyperlink made via syntax highlighter doesn't create some 'top level' anchor,
// so `anchorAt` returns nothing, we have to enumerate styles to find out a href.
QString hyperlinkAt(const QTextCursor& cursor)
{
    int cursorPos = cursor.positionInBlock() - (cursor.position() - cursor.anchor());
    for (auto& format : cursor.block().layout()->formats())
        if (format.format.isAnchor() &&
            cursorPos >= format.start &&
            cursorPos < format.start + format.length)
        {
            auto href = format.format.anchorHref();
            if (not href.isEmpty()) return href;
        }
    return QString();
}

void exportToPdf(QTextDocument* doc, const QString& fileName)
{
    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
    printer.setPageSize(QPageSize(QPageSize::A4));
#else
    printer.setPaperSize(QPrinter::A4);
#endif
    printer.setOutputFileName(fileName);

    doc->print(&printer);
}

void exportToPdfDlg(QTextEdit *editor)
{
    QString fileName = Ori::Dlg::getSaveFileName(
        qApp->tr("Export memo as PDF"),
        qApp->tr("PDF documents (*.pdf);;All files (*.*)"), "pdf");
    if (fileName.isEmpty()) return;

    exportToPdf(editor->document(), fileName);
}

bool chooseFontDlg(QTextEdit *editor)
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, editor->font(),
        qApp->activeWindow(), qApp->tr("Memo Font"),
        QFontDialog::ScalableFonts | QFontDialog::NonScalableFonts |
        QFontDialog::MonospacedFonts | QFontDialog::ProportionalFonts);
    if (!ok)
        return false;

    editor->setFont(font);
    return true;
}

void adjustDocumentWidth(QTextEdit *editor)
{
    auto sb = 1.5 * qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    editor->document()->setTextWidth(editor->width() - sb);
}

bool canClose(QLineEdit *titleEditor, std::function<bool()> save)
{
    QString memoTitle = titleEditor->text().trimmed();
    if (memoTitle.isEmpty())
        memoTitle = qApp->tr("(untitled memo)");

    int res = Ori::Dlg::yesNoCancel(qApp->tr("<b>%1</b><br/><br/>"
                                       "This memo has been changed. "
                                       "Save changes before closing?")
                                        .arg(memoTitle));
    if (res == QMessageBox::Cancel) return false;
    if (res == QMessageBox::No) return true;

    return save();
}

} // namespace TextEditHelpers
