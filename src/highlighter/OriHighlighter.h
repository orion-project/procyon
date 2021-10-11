#ifndef ORI_HIGHLIGHTER_H
#define ORI_HIGHLIGHTER_H

#include <QSyntaxHighlighter>

QT_BEGIN_NAMESPACE
class QActionGroup;
class QMenu;
QT_END_NAMESPACE

namespace Ori {
namespace Highlighter {

struct Meta
{
    QString name;
    QString title;
    QString file;

    QString displayTitle() const { return title.isEmpty() ? name : title; }
};

struct Rule
{
    QString name;
    QVector<QRegularExpression> exprs;
    QTextCharFormat format;
    QStringList terms;
    int group = 0;
    bool hyperlink = false;
    bool multiline = false;
    int fontSizeDelta = 0;
};

struct Spec
{
    Meta meta;
    QVector<Rule> rules;
};

void loadHighlighters();
QVector<Meta> availableHighlighters();
bool exists(QString name);

class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit Highlighter(QTextDocument *parent, QString name);

protected:
    void highlightBlock(const QString &text);

private:
    const struct Spec& _spec;
    QTextDocument* _document;

    int matchMultiline(const QString &text, const Rule& rule, int initialOffset);
};

class Control : public QObject
{
    Q_OBJECT

public:
    explicit Control(QObject *parent = nullptr);

    QMenu* makeMenu(QString title, QWidget* parent = nullptr);

    void showCurrent(const QString& name);
    void setEnabled(bool on);

signals:
    void selected(const QString& highlighter);

private:
    QActionGroup* _actionGroup = nullptr;

    void actionGroupTriggered(QAction* action);
};

} // namespace Highlighter
} // namespace Ori

#endif // ORI_HIGHLIGHTER_H
