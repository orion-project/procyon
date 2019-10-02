#ifndef APP_SETTING_SPAGE_H
#define APP_SETTING_SPAGE_H

#include "../AppSettings.h"

#include <QWidget>

class AppSettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit AppSettingsPage(QWidget *parent = nullptr);

private:
    QWidget* makeCategoriesList();

    AppSettings::Options _options = AppSettings::instance().options();
};

#endif // APP_SETTING_SPAGE_H
