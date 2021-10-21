#include "AppSettings.h"
#include "AppTheme.h"
#include "Utils.h"
#include "orion/tools/OriSettings.h"

#include <QApplication>
#include <QDir>
#include <QRegularExpression>

namespace AppTheme {

QString loadRawStyleSheet()
{
    return loadTextFromResource(":/style/app_main");
}

// For dev mode only
QString saveRawStyleSheet(const QString& text)
{
    // TODO: adjust for macOS
    QFile f(qApp->applicationDirPath() + "/../src/app.qss");
    if (!f.exists())
        return QString("File doesn't exist: %1").arg(f.fileName());
    if (!f.open(QFile::WriteOnly | QFile::Text | QFile::Truncate))
        return QString("Failed to open \"%1\" for writing: %2").arg(f.fileName(), f.errorString());
    if (f.write(text.toUtf8()) < 0)
        return QString("Failed to write file \"%1\": %2").arg(f.fileName(), f.errorString());
    return QString();
}

QString makeStyleSheet(const QString& rawStyleSheet)
{
    QString styleSheet = rawStyleSheet;

    // Interpolate vars
    QMap<QString, QString> vars;
    QRegularExpression varExpr(QStringLiteral("(\\$[a-zA-Z_][a-zA-Z_-]*)\\s*:\\s*(.+);"));
    auto m = varExpr.match(styleSheet);
    while (m.hasMatch())
    {
        vars[m.captured(1)] = m.captured(2);
        m = varExpr.match(styleSheet, m.capturedEnd());
    }
    styleSheet.remove(varExpr);
    auto it = vars.constBegin();
    while (it != vars.constEnd())
    {
        styleSheet.replace(it.key(), it.value());
        it++;
    }

    // Process platform-dependeent props
    auto options = QRegularExpression::CaseInsensitiveOption | QRegularExpression::MultilineOption;
    QRegularExpression propWin(QStringLiteral("^\\s*windows:(.*)$"), options);
    QRegularExpression propLinux(QStringLiteral("^\\s*linux:(.*)$"), options);
    QRegularExpression propMacos(QStringLiteral("^\\s*macos:(.*)$"), options);
#if defined(Q_OS_WIN)
    styleSheet.replace(propWin, QStringLiteral("\\1"));
    styleSheet.remove(propLinux);
    styleSheet.remove(propMacos);
#elif defined(Q_OS_LINUX)
    styleSheet.replace(propLinux, QStringLiteral("\\1"));
    styleSheet.remove(propWin);
    styleSheet.remove(propMacos);
#elif defined(Q_OS_MAC)
    styleSheet.replace(propMacos, QStringLiteral("\\1"));
    styleSheet.remove(propWin);
    styleSheet.remove(propLinux);
#endif

    return styleSheet;
}

} // namespace AppTheme
