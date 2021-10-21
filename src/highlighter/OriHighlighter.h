#ifndef ORI_HIGHLIGHTER_H
#define ORI_HIGHLIGHTER_H

#include <QWidget>
#include <QRegularExpression>
#include <QSyntaxHighlighter>

QT_BEGIN_NAMESPACE
class QActionGroup;
class QListWidget;
class QMenu;
class QPlainTextEdit;
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
    QVector<Rule> rules;

    // not empty only when spec is loaded withRawData
    // this stuff is required for highlighter editor
    QMap<int, QVariant> raw;
    enum RawData {RAW_CODE, RAW_SAMPLE, RAW_NAME_LINE, RAW_TITLE_LINE};
    QString rawCode() const { return raw[RAW_CODE].toString(); }
    QString rawSample() const { return raw[RAW_SAMPLE].toString(); }
    int rawNameLineNo() const { return raw[RAW_NAME_LINE].toInt(); }
    int rawTitleLineNo() const { return raw[RAW_TITLE_LINE].toInt(); }
    QString storableString() const;
};


class SpecStorage
{
public:
    virtual ~SpecStorage() {}
    virtual QString name() const = 0;
    virtual bool readOnly() const = 0;
    virtual QVector<Meta> loadMetas() const = 0;
    virtual QSharedPointer<Spec> loadSpec(const Meta& meta, bool withRawData = false) const = 0;
    virtual QString saveSpec(const QSharedPointer<Spec>& spec) = 0;
    virtual QString deleteSpec(const Meta& meta) = 0;
};


class DefaultStorage : public SpecStorage
{
public:
    QString name() const override { return QStringLiteral("default-storage"); }
    bool readOnly() const override { return false; }
    QVector<Meta> loadMetas() const override;
    QSharedPointer<Spec> loadSpec(const Meta &meta, bool withRawData = false) const override;
    QString saveSpec(const QSharedPointer<Spec>& spec) override;
    QString deleteSpec(const Meta&) override { return QString(); }
};

class QrcStorage : public SpecStorage
{
    QString name() const override { return QStringLiteral("qrc-storage"); }
    bool readOnly() const override { return true; }
    QVector<Meta> loadMetas() const override;
    QSharedPointer<Spec> loadSpec(const Meta &meta, bool withRawData = false) const override;
    QString saveSpec(const QSharedPointer<Spec>&) override { return QString(); }
    QString deleteSpec(const Meta&) override { return QString(); }
};

QSharedPointer<Spec> getSpec(const QString& name);
QMap<int, QString> loadSpecRaw(QSharedPointer<Spec> spec, const QString& source, QString* data, bool withRawData);
QPair<bool, bool> checkDuplicates(const Meta& meta);
QSyntaxHighlighter* createHighlighter(QPlainTextEdit* editor, const QString& name);

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
    explicit Control(QMenu* menu, QObject *parent = nullptr);

    void showManager();
    void showCurrent(const QString& name);
    void setEnabled(bool on);

    void loadMetas(const QVector<QSharedPointer<SpecStorage>>& storages);

signals:
    void selected(const QString& highlighter);
    void editorRequested(const QSharedPointer<Spec>& spec);

private:
    QMenu* _menu;
    QActionGroup* _actionGroup = nullptr;
    QPointer<class ManagerDlg> _managerDlg;

    void actionGroupTriggered(QAction* action);
    void makeMenu();
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
