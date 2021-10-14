#ifndef ORI_HIGHLIGHTER_H
#define ORI_HIGHLIGHTER_H

#include <QWidget>
#include <QRegularExpression>
#include <QSyntaxHighlighter>

QT_BEGIN_NAMESPACE
class QActionGroup;
class QMenu;
QT_END_NAMESPACE

namespace Ori {
namespace Highlighter {


class SpecStorage;


struct Meta
{
    QString name;
    QString title;
    QString source;
    QSharedPointer<SpecStorage> storage;

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
    QString code;           // not empty only when spec is loaded withRawData
    QString sample;         // not empty only when spec is loaded withRawData
    QVector<Rule> rules;
};


class SpecStorage
{
public:
    virtual ~SpecStorage() {}
    virtual bool readOnly() const = 0;
    virtual QVector<Meta> loadMetas() const = 0;
    virtual QSharedPointer<Spec> loadSpec(const QString& source, bool withRawData = false) const = 0;
};


class DefaultStorage : public SpecStorage
{
public:
    bool readOnly() const override;
    QVector<Meta> loadMetas() const override;
    QSharedPointer<Spec> loadSpec(const QString &source, bool withRawData = false) const override;

    static QSharedPointer<SpecStorage> create();
};


void loadMetas(const QVector<QSharedPointer<SpecStorage>>& storages);
QSharedPointer<Spec> getSpec(const QString& name);


class Highlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit Highlighter(QTextDocument *parent, const QSharedPointer<Spec>& spec);

protected:
    void highlightBlock(const QString &text);

private:
    QSharedPointer<Spec> _spec;
    QTextDocument* _document;

    int matchMultiline(const QString &text, const Rule& rule, int ruleIndex, int initialOffset);
};


class Control : public QObject
{
    Q_OBJECT

public:
    explicit Control(const QVector<QSharedPointer<SpecStorage>>& storages, QObject *parent = nullptr);

    QMenu* makeMenu(QString title, QWidget* parent = nullptr);

    void showCurrent(const QString& name);
    void setEnabled(bool on);
    QString currentHighlighter() const;

signals:
    void selected(const QString& highlighter);
    void editorRequested(const QSharedPointer<Spec>& spec);

private:
    QActionGroup* _actionGroup = nullptr;

    void actionGroupTriggered(QAction* action);
    void editHighlighter();
    void newHighlighter();
    void newHighlighterWithBase(const QSharedPointer<Spec>& base);
};

} // namespace Highlighter
} // namespace Ori

#endif // ORI_HIGHLIGHTER_H
