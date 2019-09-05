#include "HelpPage.h"

#include <QApplication>
#include <QMessageBox>
#include <QPushButton>

HelpPage::HelpPage(QWidget *parent) : QWidget(parent)
{

}

void HelpPage::showAbout()
{
    auto title = tr("About %1").arg(qApp->applicationName());
    auto text = tr(
                "<h2>{app} {app_ver}</h2>"
                "<p>Built: {build_date}"
                "<p>Copyright: Chunosov N.&nbsp;I. © 2017-{app_year}"
                "<p>Web: <a href='{www}'>{www}</a>"
                "<p>&nbsp;")
            .replace("{app}", qApp->applicationName())
            .replace("{app_ver}", qApp->applicationVersion())
            .replace("{app_year}", QString::number(APP_VER_YEAR))
            .replace("{build_date}", QString("%1 %2").arg(BUILDDATE).arg(BUILDTIME))
            .replace("{www}", "https://github.com/orion-project/procyon");
    QMessageBox about(QMessageBox::NoIcon, title, text, QMessageBox::Ok, qApp->activeWindow());
    about.setIconPixmap(QPixmap(":/icon/main").
        scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    auto button = about.addButton(tr("About Qt"), QMessageBox::ActionRole);
    connect(button, SIGNAL(clicked()), qApp, SLOT(aboutQt()));
    about.exec();
}
