#ifndef APP_SETTINGS_TAB_H
#define APP_SETTINGS_TAB_H

#include "../AppSettings.h"

#include <QWidget>

class AppSettingsTab : public QWidget
{
    Q_OBJECT

public:
    explicit AppSettingsTab(QWidget *parent = nullptr);

private:
    QWidget* makeCategoriesList();

    AppSettings::Options _options = AppSettings::instance().options();
};

#endif // APP_SETTINGS_TAB_H
