#ifndef APP_THEME_H
#define APP_THEME_H

#include <QString>

namespace AppTheme {

QString loadRawStyleSheet();
QString saveRawStyleSheet(const QString& text);
QString makeStyleSheet(const QString& rawStyleSheet);

} // namespace AppTheme

#endif // APP_THEME_H
