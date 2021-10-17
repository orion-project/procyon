#ifndef ORI_HIGHLIGHTER_H
#define ORI_HIGHLIGHTER_H

#include <QWidget>
#include <QRegularExpression>
#include <QSyntaxHighlighter>

QT_BEGIN_NAMESPACE
class QActionGroup;
class QListWidget;
class QMenu;
class QPushButton;
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

    QString storableString() const;
};


class SpecStorage
{
public:
    virtual ~SpecStorage() {}
    virtual QString name() const = 0;
    virtual bool readOnly() const = 0;
    virtual QVector<Meta> loadMetas() const = 0;
    virtual QSharedPointer<Spec> loadSpec(const QString& source, bool withRawData = false) const = 0;
    virtual QString saveSpec(const QSharedPointer<Spec>& spec) = 0;
};


class DefaultStorage : public SpecStorage
{
public:
    QString name() const override;
    bool readOnly() const override;
    QVector<Meta> loadMetas() const override;
    QSharedPointer<Spec> loadSpec(const QString &source, bool withRawData = false) const override;
    QString saveSpec(const QSharedPointer<Spec>& spec) override;

    static QSharedPointer<SpecStorage> create();
};


void loadMetas(const QVector<QSharedPointer<SpecStorage>>& storages);
QSharedPointer<Spec> getSpec(const QString& name);
QMap<int, QString> loadSpecRaw(QSharedPointer<Spec> spec, const QString& source, QString* data, bool withRawData);

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

    void showManager();
    void showCurrent(const QString& name);
    void setEnabled(bool on);

signals:
    void selected(const QString& highlighter);
    void editorRequested(const QSharedPointer<Spec>& spec);

private:
    QActionGroup* _actionGroup = nullptr;
    QPointer<class ManagerDlg> _managerDlg;

    void actionGroupTriggered(QAction* action);
};


class ManagerDlg : public QWidget
{
private:
    QListWidget *_specList;
    Control *_parent;

    ManagerDlg(Control *parent);
    friend class Control;

    QString selectedSpecName() const;

    void editHighlighter();
    void createHighlighterEmpty();
    void createHighlighterCopy();
    void deleteHighlighter();
    void newHighlighterWithBase(const Meta& meta);
};

} // namespace Highlighter
} // namespace Ori

#endif // ORI_HIGHLIGHTER_H
