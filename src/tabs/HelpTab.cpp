#include "HelpTab.h"

#include "TabHelpers.h"
#include "../AppSettings.h"
#include "../markdown/MarkdownHelper.h"
#include "../widgets/MemoTextBrowser.h"

#include "helpers/OriLayouts.h"
#include "widgets/OriLabels.h"

#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QDialog>
#include <QLabel>
#include <QUrl>
#include <QToolBar>

using namespace Ori::Layouts;

static QString loadTextFromResource(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Unable to open resource file" << fileName << file.errorString();
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}


HelpTab::HelpTab(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(tr("Reference Manual"));
    setWindowIcon(QIcon(":/icon/help"));

    auto browser = new MemoTextBrowser;
    browser->document()->setDefaultStyleSheet(AppSettings::instance().markdownCss());
    browser->document()->setDocumentMargin(10);
    browser->setHtml(MarkdownHelper::markdownToHtml(loadTextFromResource(":/docs/help")));

    auto titleEditor = TabHelpers::makeTitleEditor(windowTitle());

    auto toolbar = new QToolBar;
    toolbar->addAction(QIcon(":/toolbar/close"), tr("Close Tab"), [this]{ deleteLater(); });

    auto toolPanel = TabHelpers::makeHeaderPanel({titleEditor, toolbar});

    Ori::Layouts::LayoutV({toolPanel, browser}).setMargin(0).setSpacing(0).useFor(this);
}

void HelpTab::showAbout()
{
    auto w = new QDialog;
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->setWindowTitle(tr("About %1").arg(qApp->applicationName()));

    QPixmap bckgnd(":/style/about");
    w->setMaximumSize(bckgnd.size());
    w->setMinimumSize(bckgnd.size());
    w->resize(bckgnd.size());

    auto p = w->palette();
    p.setBrush(QPalette::Window, QBrush(bckgnd));
    w->setPalette(p);

    auto f = w->font();
    //f.setFamily("sans-serif");

    auto labelVersion = new QLabel(qApp->applicationVersion());
    f.setPixelSize(30);
    labelVersion->setFont(f);
    labelVersion->setStyleSheet("color:#2e2f33");

    f.setPixelSize(18);

    auto labelDate = new QLabel(APP_VER_DATE);
    labelDate->setStyleSheet("color:#2e2f33");
    labelDate->setFont(f);

    f.setPixelSize(15);

    auto labelQt = new Ori::Widgets::Label(QString("Powered by Qt %1").arg(QT_VERSION_STR));
    connect(labelQt, &Ori::Widgets::Label::clicked, []{ qApp->aboutQt(); });
    labelQt->setToolTip(tr("About Qt"));
    labelQt->setCursor(Qt::PointingHandCursor);
    labelQt->setStyleSheet("color:#000ace;");
    labelQt->setFont(f);

    auto labelCopyright = new QLabel(QString("Chunosov N.I. © 2017-%1").arg(APP_VER_YEAR));
    labelCopyright->setStyleSheet("color:#2e2f33");
    labelCopyright->setFont(f);

    QString address("http://github.com/orion-project/procyon");
    auto labelWebsite = new Ori::Widgets::Label("github.com/orion-project/procyon");
    connect(labelWebsite, &Ori::Widgets::Label::clicked, [address]{ QDesktopServices::openUrl(QUrl(address)); });
    labelWebsite->setToolTip(address);
    labelWebsite->setCursor(Qt::PointingHandCursor);
    labelWebsite->setStyleSheet("color:#000ace;");
    labelWebsite->setFont(f);

    LayoutV({
        Space(90),
        LayoutH({Stretch(), labelVersion}),
        LayoutH({Stretch(), labelDate}),
        Stretch(),
        LayoutH({Stretch(), labelQt}),
        Space(8),
        LayoutH({Stretch(), labelCopyright}),
        Space(8),
        LayoutH({Stretch(), labelWebsite}),
    }).setMargin(12).setSpacing(0).useFor(w);

    w->exec();
}

void HelpTab::visitHomePage()
{
    QDesktopServices::openUrl(QUrl("https://github.com/orion-project/procyon"));
}

void HelpTab::sendBugReport()
{
    QDesktopServices::openUrl(QUrl("https://github.com/orion-project/procyon/issues/new"));
}
