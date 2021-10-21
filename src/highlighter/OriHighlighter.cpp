#include "OriHighlighter.h"
#include "../widgets/PopupMessage.h"

#include <QActionGroup>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QListWidget>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QTextDocument>
#include <QBoxLayout>

#include "orion/helpers/OriDialogs.h"

namespace Ori {
namespace Highlighter {

//------------------------------------------------------------------------------
//                                    Spec
//------------------------------------------------------------------------------

QString Spec::storableString() const
{
    return rawCode().trimmed() + "\n\n---\n" + rawSample().trimmed();
}

//------------------------------------------------------------------------------
//                                 SpecLoader
//------------------------------------------------------------------------------

struct SpecLoader
{
private:
    QString source;
    QTextStream& stream;
    int lineNo = 0;
    QString key, val;
    QStringList code;
    QStringList sample;
    int sampleLineNo = -1;
    bool withRawData = false;
    QMap<int, QString> warnings;
    QMap<QString, int> ruleStarts;

    void warning(const QString& msg, int overrideLineNo = 0)
    {
        int reportedLineNo = (overrideLineNo > 0) ? overrideLineNo : lineNo;
        qWarning() << "Highlighter" << source << "| line" << reportedLineNo << "|" << msg;
        warnings[reportedLineNo] = msg;
    }

    bool readLine()
    {
        lineNo++;
        auto line = stream.readLine();

        if (sampleLineNo >= 0)
        {
            val = line;
            sampleLineNo++;
            return true;
        }

        if (line.startsWith(QStringLiteral("---")))
        {
            sampleLineNo = 0;
            return true;
        }

        code << line;

        line = line.trimmed();
        if (line.isEmpty() || line[0] == '#')
            return false;
        auto keyLen = line.indexOf(':');
        if (keyLen < 1)
        {
            warning("Key not found");
            return false;
        }
        key = line.first(keyLen).trimmed();
        val = line.sliced(keyLen+1).trimmed();
        //qDebug() << "Highlighter" << source << "| line" << lineNo << "| key" << key << "| value" << val;
        return true;
    }

    void finalizeRule(Rule& rule, Spec* spec, const QRegularExpression::PatternOptions& opts)
    {
        if (!rule.terms.isEmpty())
        {
            rule.exprs.clear();
            for (const auto& term : rule.terms)
                rule.exprs << QRegularExpression(QString("\\b%1\\b").arg(term));
        }
        if (rule.multiline)
        {
            if (rule.exprs.isEmpty())
            {
                warning(QStringLiteral("Must be at least one \"expr\" when multiline"), ruleStarts[rule.name]);
                rule.multiline = false;
            }
            else if (rule.exprs.size() == 1)
                rule.exprs << QRegularExpression(rule.exprs.first());
            else if (rule.exprs.size() > 2)
                rule.exprs.resize(2);
        }
        for (auto& expr : rule.exprs)
            expr.setPatternOptions(opts);
        spec->rules << rule;
    }

public:
    explicit SpecLoader(const QString& source, QTextStream& stream, bool withRawData)
        : source(source), stream(stream), withRawData(withRawData)
    {}

    bool loadMeta(Meta& meta, Spec* spec = nullptr)
    {
        bool suffice = false;
        while (!stream.atEnd())
        {
            if (!readLine())
                continue;
            if (sampleLineNo >= 0)
            {
                break;
            }
            else if (key == QStringLiteral("name"))
            {
                meta.name = val;
                suffice = true;
                if (spec && withRawData)
                    spec->raw[Spec::RAW_NAME_LINE] = lineNo;
            }
            else if (key == QStringLiteral("title"))
            {
                meta.title = val;
                if (spec && withRawData)
                    spec->raw[Spec::RAW_TITLE_LINE] = lineNo;
            }
            else if (key == QStringLiteral("rule"))
            {
                break;
            }
            else warning(QStringLiteral("Unknown key"));
        }
        if (!suffice)
            warning(QStringLiteral("Not all required top-level properties set, required: \"name\""), 1);
        return suffice;
    }

    QMap<int, QString> loadSpec(Spec* spec)
    {
        // ! Don't clear meta.source and meta.storage
        // ! they are parameters of the loading
        spec->meta.name.clear();
        spec->meta.title.clear();
        spec->raw.clear();
        spec->rules.clear();

        if (!loadMeta(spec->meta, spec))
            return warnings;

        Rule rule;
        rule.name = val;
        QRegularExpression::PatternOptions opts;

        while (!stream.atEnd())
        {
            if (!readLine())
                continue;
            if (sampleLineNo == 0)
            {
                if (!withRawData) break;
                // rules-to-sample separator, just skip it
            }
            else if (sampleLineNo > 0)
            {
                sample << val;
            }
            else if (key == QStringLiteral("rule"))
            {
                finalizeRule(rule, spec, opts);

                for (const auto& r : spec->rules)
                    if (r.name == val)
                    {
                        warning(QStringLiteral("Dupilicated rule name"));
                        warning(QStringLiteral("Dupilicated rule name"), ruleStarts[r.name]);
                    }
                rule = Rule();
                rule.name = val;
                ruleStarts[val] = lineNo;
                opts = QRegularExpression::PatternOptions();
            }
            else if (key == QStringLiteral("expr"))
            {
                if (rule.terms.isEmpty())
                {
                    QRegularExpression expr(val);
                    if (!expr.isValid())
                        warning(QStringLiteral("Invalid expression"));
                    else
                        rule.exprs << expr;
                }
                else warning(QStringLiteral("Can't have \"expr\" and \"terms\" in the same rule"));
            }
            else if (key == QStringLiteral("color"))
            {
                QColor c(val);
                if (!c.isValid())
                    warning(QStringLiteral("Invalid color value"));
                else
                    rule.format.setForeground(c);
            }
            else if (key == QStringLiteral("back"))
            {
                QColor c(val);
                if (!c.isValid())
                    warning(QStringLiteral("Invalid color value"));
                else
                    rule.format.setBackground(c);
            }
            else if (key == QStringLiteral("group"))
            {
                bool ok;
                int group = val.toInt(&ok);
                if (!ok)
                    warning(QStringLiteral("Invalid integer value"));
                else
                    rule.group = group;
            }
            else if (key == QStringLiteral("style"))
            {
                for (const auto& style : val.split(',', Qt::SkipEmptyParts))
                {
                    auto s = style.trimmed();
                    if (s == QStringLiteral("bold"))
                        rule.format.setFontWeight(QFont::Bold);
                    else if (s == QStringLiteral("italic"))
                        rule.format.setFontItalic(true);
                    else if (s == QStringLiteral("underline"))
                        rule.format.setFontUnderline(true);
                    else if (s == QStringLiteral("strikeout"))
                        rule.format.setFontStrikeOut(true);
                    else if (s == QStringLiteral("hyperlink"))
                    {
                        rule.format.setAnchor(true);
                        rule.hyperlink = true;
                    }
                    else warning(QStringLiteral("Unknown style ") + s);
                }
            }
            else if (key == QStringLiteral("opts"))
            {
                for (const auto& style : val.split(',', Qt::SkipEmptyParts))
                {
                    auto s = style.trimmed();
                    if (s == QStringLiteral("multiline"))
                        rule.multiline = true;
                    else if (s == QStringLiteral("ignore-case"))
                        opts.setFlag(QRegularExpression::CaseInsensitiveOption);
                    else warning(QStringLiteral("Unknown option ") + s);
                }
            }
            else if (key == QStringLiteral("terms"))
            {
                if (rule.exprs.isEmpty())
                {
                    for (const auto& term : val.split(',', Qt::SkipEmptyParts))
                        rule.terms << term.trimmed();
                }
                else warning(QStringLiteral("Can't have \"expr\" and \"terms\" in the same rule"));
            }
            else warning(QStringLiteral("Unknown key"));
        }
        finalizeRule(rule, spec, opts);
        if (withRawData)
        {
            spec->raw[Spec::RAW_CODE] = code.join('\n');
            spec->raw[Spec::RAW_SAMPLE] = sample.join('\n');
        }
        return warnings;
    }
};

QMap<int, QString> loadSpecRaw(QSharedPointer<Spec> spec, const QString& source, QString* data, bool withRawData)
{
    QTextStream stream(data);
    SpecLoader loader(source, stream, withRawData);
    return loader.loadSpec(spec.get());
}

QSharedPointer<Spec> createSpec(const Meta& meta, bool withRawData)
{
    auto spec = meta.storage->loadSpec(meta, withRawData);
    if (!spec) return QSharedPointer<Spec>();
    spec->meta.source = meta.source;
    spec->meta.storage = meta.storage;
    return spec;
}

//------------------------------------------------------------------------------
//                               DefaultStorage
//------------------------------------------------------------------------------

QVector<Meta> DefaultStorage::loadMetas() const
{
    QVector<Meta> metas;
    QDir dir(qApp->applicationDirPath() + "/syntax");
#ifdef Q_OS_MAC
    if (!dir.exists())
    {
        // Look near the application bundle, it is for development mode
        return QDir(qApp->applicationDirPath() % "/../../../syntax");
    }
#endif
    if (!dir.exists())
    {
        qWarning() << "Syntax highlighter directory doesn't exist" << dir.path();
        return metas;
    }
    qDebug() << "Hightlighters::DefaultStorage: dir" << dir.path();
    for (auto& fileInfo : dir.entryInfoList())
    {
        if (fileInfo.fileName().endsWith(".phl"))
        {
            auto fileName = fileInfo.absoluteFilePath();
            QFile file(fileName);
            if (!file.open(QFile::ReadOnly | QFile::Text))
            {
                qWarning() << "Highlighter::DefaultStorage.loadMetas" << fileName << "|" << file.errorString();
                continue;
            }
            QTextStream stream(&file);
            SpecLoader loader(fileName, stream, false);
            Meta meta;
            if (loader.loadMeta(meta))
            {
                meta.source = fileName;
                metas << meta;
            }
            else
                qWarning() << "Highlighters::DefaultStorage: meta not loaded" << fileName;
        }
    }
    return metas;
}

QSharedPointer<Spec> DefaultStorage::loadSpec(const Meta &meta, bool withRawData) const
{
    QFile file(meta.source);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        qWarning() << "Highlighter::DefaultStorage.loadSpec" << meta.source << "|" << file.errorString();
        return QSharedPointer<Spec>();
    }
    QTextStream stream(&file);
    QSharedPointer<Spec> spec(new Spec());
    SpecLoader loader(meta.source, stream, withRawData);
    loader.loadSpec(spec.get());
    return spec;
}

QString DefaultStorage::saveSpec(const QSharedPointer<Spec>& spec)
{
    QFile file(spec->meta.source);
    if (!file.open(QFile::WriteOnly | QFile::Text | QFile::Truncate))
        return QString("Failed to open highlighter file \"%1\" for writing: %2").arg(spec->meta.source, file.errorString());
    if (file.write(spec->storableString().toUtf8()) == -1)
        return QString("Failed to write highlighter file \"%1\": %2").arg(spec->meta.source, file.errorString());
    return "";
}

//------------------------------------------------------------------------------
//                              QrcStorage
//------------------------------------------------------------------------------

QVector<Meta> QrcStorage::loadMetas() const
{
    QVector<Meta> metas;
    QVector<QString> files({
        QStringLiteral(":/syntax/css"),
        QStringLiteral(":/syntax/phl"),
        QStringLiteral(":/syntax/procyon"),
        QStringLiteral(":/syntax/python"),
        QStringLiteral(":/syntax/qss"),
        QStringLiteral(":/syntax/sql"),
    });

    for (const auto& fileName : files)
    {
        QFile file(fileName);
        if (!file.open(QFile::ReadOnly | QFile::Text))
        {
            qWarning() << "Highlighter::QrcStorage.loadMetas" << fileName << "|" << file.errorString();
            continue;
        }
        QTextStream stream(&file);
        SpecLoader loader(fileName, stream, false);
        Meta meta;
        if (loader.loadMeta(meta))
        {
            meta.source = fileName;
            metas << meta;
        }
        else
            qWarning() << "Highlighters::QrcStorage: meta not loaded" << fileName;
    }
    return metas;
}

QSharedPointer<Spec> QrcStorage::loadSpec(const Meta &meta, bool withRawData) const
{
    QFile file(meta.source);
    if (!file.open(QFile::ReadOnly | QFile::Text))
    {
        qWarning() << "Highlighter::QrcStorage.loadSpec" << meta.source << "|" << file.errorString();
        return QSharedPointer<Spec>();
    }
    QTextStream stream(&file);
    QSharedPointer<Spec> spec(new Spec());
    SpecLoader loader(meta.source, stream, withRawData);
    loader.loadSpec(spec.get());
    return spec;
}

//------------------------------------------------------------------------------
//                                 SpecCache
//------------------------------------------------------------------------------

struct SpecCache
{
    QMap<QString, Meta> allMetas;
    QMap<QString, QSharedPointer<Spec>> loadedSpecs;
    QSharedPointer<SpecStorage> customStorage;

    QSharedPointer<Spec> getSpec(QString name)
    {
        if (!allMetas.contains(name))
        {
            qWarning() << "Highlighters::SpecCache: unknown name" << name;
            return QSharedPointer<Spec>();
        }
        if (!loadedSpecs.contains(name))
        {
            const auto& meta = allMetas[name];
            if (!meta.storage)
            {
                qWarning() << "Highlighters::SpecCache: storage not set" << name;
                return QSharedPointer<Spec>();
            }
            auto spec = createSpec(meta, false);
            if (!spec) return QSharedPointer<Spec>();
            loadedSpecs[name] = spec;
        }
        return loadedSpecs[name];
    }
};

static SpecCache& specCache()
{
    static SpecCache cache;
    return cache;
}

QSharedPointer<Spec> getSpec(const QString& name)
{
    return specCache().getSpec(name);
}

QPair<bool, bool> checkDuplicates(const Meta& meta)
{
    bool name = false;
    bool title = false;
    const auto& metas = specCache().allMetas;
    auto it = metas.constBegin();
    while (it != metas.constEnd())
    {
        const auto& m = it.value();
        if (m.source != meta.source)
        {
            if (!name && m.name == meta.name)
                name = true;
            if (!title && m.title == meta.title)
                title = true;
            if (name && title)
                break;
        }
        it++;
    }
    return {name, title};
}

//------------------------------------------------------------------------------
//                                 Highlighter
//------------------------------------------------------------------------------

QSyntaxHighlighter* createHighlighter(QPlainTextEdit* editor, const QString& name)
{
    auto hl = Ori::Highlighter::getSpec(name);
    return hl ? new Highlighter(editor->document(), hl) : nullptr;
}

Highlighter::Highlighter(QTextDocument *parent, const QSharedPointer<Spec>& spec)
    : QSyntaxHighlighter(parent), _spec(spec), _document(parent)
{
    setObjectName(spec->meta.name);
}

void Highlighter::highlightBlock(const QString &text)
{
    bool hasMultilines = false;
    for (const auto& rule : _spec->rules)
    {
        if (rule.multiline && rule.exprs.size() >= 1)
        {
            hasMultilines = true;
            continue;
        }
        for (const auto& expr : rule.exprs)
        {
            auto m = expr.match(text);
            while (m.hasMatch())
            {
                int pos = m.capturedStart(rule.group);
                int length = m.capturedLength(rule.group);

                // Font style is applied correctly but highlighter can't make anchors and apply tooltips.
                // We do it manually overriding event handlers in MemoEditor.
                // There is the bug but seems nobody cares: https://bugreports.qt.io/browse/QTBUG-21553
                if (rule.hyperlink)
                {
                    QTextCharFormat format(rule.format);
                    format.setAnchorHref(m.captured(rule.group));
                    setFormat(pos, length, format);
                }
                else if (rule.fontSizeDelta != 0)
                {
                    QTextCharFormat format(rule.format);
                    format.setFontPointSize(_document->defaultFont().pointSize() + rule.fontSizeDelta);
                    setFormat(pos, length, format);
                }
                else
                    setFormat(pos, length, rule.format);
                m = expr.match(text, pos + length);
            }
        }
    }
    if (hasMultilines)
    {
        int offset = 0;
        setCurrentBlockState(-1);
        int size = _spec->rules.size();
        for (int i = 0; i < size; i++)
        {
            const auto& rule = _spec->rules.at(i);
            if (!rule.multiline) continue;
            offset = matchMultiline(text, rule, i, offset);
            if (offset < 0) break;
        }
    }
}

int Highlighter::matchMultiline(const QString &text, const Rule& rule, int ruleIndex, int initialOffset)
{
    const auto& exprBeg = rule.exprs[0];
    const auto& exprEnd = rule.exprs[1];
    QRegularExpressionMatch m;

    //qDebug() << rule.name << previousBlockState() << "|" << initialOffset << "|" << text;

    int start = 0;
    int offset = initialOffset;
    bool matchEnd = previousBlockState() == ruleIndex;
    while (true)
    {
        m = (matchEnd ? exprEnd : exprBeg).match(text, offset);
        if (m.hasMatch())
        {
            if (matchEnd)
            {
                setFormat(start, m.capturedEnd()-start, rule.format);
                setCurrentBlockState(-1);
                matchEnd = false;
                //qDebug() << "    has-match(end)" << start << m.capturedEnd()-start;
            }
            else
            {
                start = m.capturedStart();
                matchEnd = true;
                //qDebug() << "    has-match(beg)" << start;
            }
            offset = m.capturedEnd();
            //qDebug() << "    offset" << offset;
        }
        else
        {
            if (matchEnd)
            {
                //qDebug() << "    no-match(end)" << start << text.length()-start;
                setFormat(start, text.length()-start, rule.format);
                setCurrentBlockState(ruleIndex);
                offset = -1;
            }
            else
            {
                //qDebug() << "    no-match(beg)";
            }
            break;
        }
    }
    //qDebug() << "    return" << offset;
    return offset;
}

//------------------------------------------------------------------------------
//                                 Control
//------------------------------------------------------------------------------

Control::Control(QMenu *menu, QObject *parent) : QObject(parent), _menu(menu)
{
}

void Control::loadMetas(const QVector<QSharedPointer<SpecStorage>>& storages)
{
    if (_managerDlg)
        _managerDlg->close();

    auto& cache = specCache();
    cache.allMetas.clear();
    cache.loadedSpecs.clear();
    for (const auto& storage : storages)
    {
        // The first writable storage becomes a default storage
        // for new highlighters, this is enough for now
        if (!storage->readOnly() && !cache.customStorage)
            cache.customStorage = storage;

        for (auto& meta : storage->loadMetas())
        {
            if (cache.allMetas.contains(meta.name))
            {
                const auto& existedMeta = cache.allMetas[meta.name];
                qWarning() << "Highlighter is already registered" << existedMeta.name << existedMeta.source
                           << (existedMeta.storage ? existedMeta.storage->name() : QString("null-storage"));
                continue;
            }
            meta.storage = storage;
            cache.allMetas[meta.name] = meta;
            qDebug() << "Highlighter registered" << meta.name << meta.source << meta.storage->name();
        }
    }

    makeMenu();
}

void Control::makeMenu()
{
    if (_actionGroup) delete _actionGroup;

    _actionGroup = new QActionGroup(this);
    connect(_actionGroup, &QActionGroup::triggered, this, &Control::actionGroupTriggered);

    auto actionNone = new QAction(tr("None"), this);
    actionNone->setCheckable(true);
    _actionGroup->addAction(actionNone);

    const auto& allMetas = specCache().allMetas;
    auto it = allMetas.constBegin();
    while (it != allMetas.constEnd())
    {
        const auto& meta = it.value();
        auto actionDict = new QAction(meta.displayTitle(), _actionGroup);
        actionDict->setCheckable(true);
        actionDict->setData(meta.name);
        it++;
    }

    _menu->clear();
    _menu->addActions(_actionGroup->actions());
}

void Control::showCurrent(const QString& name)
{
    if (!_actionGroup) return;
    for (const auto& action : _actionGroup->actions())
        if (action->data().toString() == name)
        {
            action->setChecked(true);
            break;
        }
}

void Control::setEnabled(bool on)
{
    _actionGroup->setEnabled(on);
}

void Control::actionGroupTriggered(QAction* action)
{
    emit selected(action->data().toString());
}

void Control::showManager()
{
    if (!_managerDlg)
        _managerDlg = new ManagerDlg(this);
    _managerDlg->show();
    _managerDlg->activateWindow();
}

//------------------------------------------------------------------------------
//                                 Control
//------------------------------------------------------------------------------

ManagerDlg::ManagerDlg(Control *parent) : QWidget(), _parent(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

    _specList = new QListWidget;
    _specList->setObjectName("pages_list");
    auto it = specCache().allMetas.constBegin();
    while (it != specCache().allMetas.constEnd())
    {
        const auto& meta = it.value();
        QString title = meta.displayTitle();
        if (!meta.storage)
            title += " (invalid, no storage)";
        else if (meta.storage->readOnly())
            title += " (built-in, read only)";
        auto item = new QListWidgetItem(title, _specList);
        item->setData(Qt::UserRole, meta.name);
        it++;
    }

    auto buttonEdit = new QPushButton(tr("Edit"));
    connect(buttonEdit, &QPushButton::clicked, this, &ManagerDlg::editHighlighter);

    auto buttonNewEmpty = new QPushButton(tr("New Empty"));
    connect(buttonNewEmpty, &QPushButton::clicked, this, &ManagerDlg::createHighlighterEmpty);

    auto buttonNewCopy = new QPushButton(tr("New As Copy"));
    connect(buttonNewCopy, &QPushButton::clicked, this, &ManagerDlg::createHighlighterCopy);

    auto buttonDelete = new QPushButton(tr("Delete"));
    connect(buttonDelete, &QPushButton::clicked, this, &ManagerDlg::deleteHighlighter);

    auto buttonClose = new QPushButton(tr("Close"));
    connect(buttonClose, &QPushButton::clicked, this, &ManagerDlg::close);

    auto layoutButtons = new QVBoxLayout;
    layoutButtons->addWidget(buttonEdit);
    layoutButtons->addWidget(buttonNewEmpty);
    layoutButtons->addWidget(buttonNewCopy);
    layoutButtons->addSpacing(50);
    layoutButtons->addWidget(buttonDelete);
    layoutButtons->addSpacing(50);
    layoutButtons->addStretch();
    layoutButtons->addWidget(buttonClose);

    auto layout = new QHBoxLayout(this);
    layout->addWidget(_specList);
    layout->addLayout(layoutButtons);
    layout->setContentsMargins(7, 10, 10, 10);
}

QString ManagerDlg::selectedSpecName() const
{
    auto item = _specList->currentItem();
    return item ? item->data(Qt::UserRole).toString() : QString();
}

void ManagerDlg::editHighlighter()
{
    auto& cache = specCache();
    auto name = selectedSpecName();
    if (name.isEmpty())
        return Ori::Dlg::info(tr("No highlighter is selected"));

    const auto& meta = cache.allMetas[name];
    if (!meta.storage)
        return Ori::Dlg::warning(tr("Hihghlighter storage is not set"));

    if (!meta.storage->readOnly())
    {
        // reload spec with code and sample text
        auto fullSpec = createSpec(meta, true);
        if (!fullSpec)
            return Ori::Dlg::error("Failed to load highlighter");
        emit _parent->editorRequested(fullSpec);
        close();
        return;
    }

    if (Ori::Dlg::yes(tr("Highlighter \"%1\" is built-in and can not be edited. "
                         "Do you want to create a new highlighter on its base instead?"
                         ).arg(meta.displayTitle())))
    {
        newHighlighterWithBase(meta);
        close();
    }
}

void ManagerDlg::createHighlighterEmpty()
{
    auto& cache = specCache();
    QSharedPointer<Spec> spec(new Spec);
    spec->meta.storage = cache.customStorage;
    emit _parent->editorRequested(spec);
    close();
}

void ManagerDlg::createHighlighterCopy()
{
    auto& cache = specCache();
    auto name = selectedSpecName();
    if (name.isEmpty())
        return Ori::Dlg::info(tr("No highlighter is selected"));

    const auto& meta = cache.allMetas[name];
    if (!meta.storage)
        return Ori::Dlg::warning(tr("Hihghlighter storage is not set"));

    newHighlighterWithBase(meta);
    close();
}

void ManagerDlg::newHighlighterWithBase(const Meta &meta)
{
    auto spec = createSpec(meta, true);
    if (!spec)
    {
        Ori::Dlg::error("Failed to load base highlighter");
        spec.reset(new Spec());
    }
    spec->meta.name = "";
    spec->meta.source = "";
    spec->meta.title = "";
    spec->meta.storage = specCache().customStorage;
    emit _parent->editorRequested(spec);
}

void ManagerDlg::deleteHighlighter()
{
    auto& cache = specCache();
    auto name = selectedSpecName();
    if (name.isEmpty())
        return Ori::Dlg::info(tr("No highlighter is selected"));

    const auto& meta = cache.allMetas[name];
    if (!meta.storage)
        return Ori::Dlg::warning(tr("Hihghlighter storage is not set"));

    if (meta.storage->readOnly())
        return Ori::Dlg::info(tr("Highlighter \"%1\" is built-in and can not be deleted").arg(meta.displayTitle()));

    if (!Ori::Dlg::yes(tr("Delete highlighter \"%1\"?").arg(meta.displayTitle())))
        return;

    auto res = meta.storage->deleteSpec(meta);
    if (!res.isEmpty())
        Ori::Dlg::error(tr("There is an error during highlighter deletion\n\n%1").arg(res));

    PopupMessage::affirm(tr("Highlighter successfully deleted\n\n"
        "Application is required to be restarted to reflect changes"));

    delete _specList->currentItem();
    _specList->setCurrentItem(_specList->item(0));
}

} // namespace Highlighter
} // namespace Ori
