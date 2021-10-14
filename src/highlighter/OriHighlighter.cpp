#include "OriHighlighter.h"

#include <QActionGroup>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QRegularExpression>
#include <QTextDocument>

#include "orion/helpers/OriDialogs.h"

namespace Ori {
namespace Highlighter {

//------------------------------------------------------------------------------
//                                 SpecLoader
//------------------------------------------------------------------------------

struct SpecLoader
{
private:
    QFile file;
    QTextStream stream;
    int lineNo = 0;
    QString key, val;
    QStringList code;
    QStringList sample;
    int sampleLineNo = -1;
    bool withRawData = false;

    void warning(QString msg)
    {
        qWarning() << "Highlighter" << file.fileName() << "| line" << lineNo << "|" << msg;
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
            warning("key not found");
            return false;
        }
        key = line.first(keyLen).trimmed();
        val = line.sliced(keyLen+1).trimmed();
        //qDebug() << "Highlighter" << file.fileName() << "| line" << lineNo << "| key" << key << "| value" << val;
        return true;
    }

    void finalizeRule(Spec* spec, Rule& rule)
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
                qWarning() << "Highlighter" << file.fileName() << "| rule"
                           << rule.name << "| must be at least one \"expr\" when multiline";
                rule.multiline = false;
            }
            else if (rule.exprs.size() == 1)
                rule.exprs << QRegularExpression(rule.exprs.first());
            else if (rule.exprs.size() > 2)
                rule.exprs.resize(2);
        }
        spec->rules << rule;
    }

public:
    SpecLoader(QString fileName, bool withRawData)
    {
        this->withRawData = withRawData;
        file.setFileName(fileName);
        if (!file.open(QFile::ReadOnly | QFile::Text))
        {
            qWarning() << "Unable to open" << fileName << "|" << file.errorString();
            return;
        }
        stream.setDevice(&file);
    }

    bool loadMeta(Meta& meta)
    {
        if (!file.isOpen())
            return false;

        bool suffice = false;
        while (!stream.atEnd())
        {
            if (!readLine())
                continue;
            if (sampleLineNo >= 0)
            {
                return suffice;
            }
            else if (key == QStringLiteral("name"))
            {
                meta.name = val;
                suffice = true;
            }
            else if (key == QStringLiteral("title"))
            {
                meta.title = val;
            }
            else if (key == QStringLiteral("rule"))
            {
                return suffice;
            }
            else warning("unknown key");
        }
        return false;
    }

    void load(Spec* spec)
    {
        if (!loadMeta(spec->meta))
            return;

        Rule rule;
        rule.name = val;

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
                finalizeRule(spec, rule);
                rule = Rule();
                rule.name = val;
            }
            else if (key == QStringLiteral("expr"))
            {
                if (rule.terms.isEmpty())
                {
                    QRegularExpression expr(val);
                    if (!expr.isValid())
                        warning("invalid expression");
                    else
                        rule.exprs << expr;
                }
                else warning("can't have \"expr\" and \"terms\" in the same rule");
            }
            else if (key == QStringLiteral("color"))
            {
                QColor c(val);
                if (!c.isValid())
                    warning("invalid color value");
                else
                    rule.format.setForeground(c);
            }
            else if (key == QStringLiteral("back"))
            {
                QColor c(val);
                if (!c.isValid())
                    warning("invalid color value");
                else
                    rule.format.setBackground(c);
            }
            else if (key == QStringLiteral("group"))
            {
                bool ok;
                int group = val.toInt(&ok);
                if (!ok)
                    warning("invalid integer value");
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
                    else if (s == QStringLiteral("multiline"))
                        rule.multiline = true;
                    else warning("unknown style " + s);
                }
            }
            else if (key == QStringLiteral("terms"))
            {
                if (rule.exprs.isEmpty())
                {
                    for (const auto& term : val.split(',', Qt::SkipEmptyParts))
                        rule.terms << term.trimmed();
                }
                else warning("can't have \"expr\" and \"terms\" in the same rule");
            }
            else warning("unknown key");
        }
        finalizeRule(spec, rule);
        if (withRawData)
        {
            spec->code = code.join('\n');
            spec->sample = sample.join('\n');
        }
    }
};

//------------------------------------------------------------------------------
//                              DefaultSpecStorage
//------------------------------------------------------------------------------

QSharedPointer<SpecStorage> DefaultStorage::create()
{
    return QSharedPointer<SpecStorage>(new DefaultStorage());
}

bool DefaultStorage::readOnly() const
{
    return false;
}

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
    qDebug() << "Hightlighters dir" << dir.path();
    for (auto& fileInfo : dir.entryInfoList())
    {
        if (fileInfo.fileName().endsWith(".phl"))
        {
            Meta meta;
            auto filename = fileInfo.absoluteFilePath();
            SpecLoader loader(filename, false);
            if (loader.loadMeta(meta))
            {
                meta.source = filename;
                metas << meta;
            }
        }
    }
    return metas;
}

QSharedPointer<Spec> DefaultStorage::loadSpec(const QString& source, bool withRawData) const
{
    QSharedPointer<Spec> spec(new Spec());
    SpecLoader loader(source, withRawData);
    loader.load(spec.get());
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
            qWarning() << "Unknown highlighter" << name;
            return QSharedPointer<Spec>();
        }
        if (!loadedSpecs.contains(name))
        {
            const auto& meta = allMetas[name];
            auto spec = meta.storage->loadSpec(meta.source);
            spec->meta.source = meta.source;
            spec->meta.storage = meta.storage;
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

void loadMetas(const QVector<QSharedPointer<SpecStorage>>& storages)
{
    auto& cache = specCache();
    for (const auto& storage : storages)
    {
        // The first writable storage becomes a default storage
        // for new highlighters, this enough for now
        if (!storage->readOnly() && !cache.customStorage)
            cache.customStorage = storage;

        for (auto& meta : storage->loadMetas())
        {
            if (cache.allMetas.contains(meta.name))
            {
                qWarning() << "Highlighter is already registered" << meta.name;
                continue;
            }
            meta.storage = storage;
            cache.allMetas[meta.name] = meta;
            qDebug() << "Highlighter registered" << meta.name << meta.source;
        }
    }
}

QSharedPointer<Spec> getSpec(const QString& name)
{
    return specCache().getSpec(name);
}

//------------------------------------------------------------------------------
//                                 Highlighter
//------------------------------------------------------------------------------

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
            if (m.hasMatch())
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
                setCurrentBlockState(0);
                matchEnd = false;
            }
            else
            {
                start = m.capturedStart();
                matchEnd = true;
            }
            offset = m.capturedEnd();
        }
        else
        {
            if (matchEnd)
            {
                setFormat(start, text.length()-start, rule.format);
                setCurrentBlockState(ruleIndex);
                offset = -1;
            }
            break;
        }
    }
    return offset;
}

//------------------------------------------------------------------------------
//                                 Control
//------------------------------------------------------------------------------

Control::Control(const QVector<QSharedPointer<SpecStorage>>& storages, QObject *parent) : QObject(parent)
{
    _actionGroup = new QActionGroup(parent);
    _actionGroup->setExclusive(true);
    connect(_actionGroup, &QActionGroup::triggered, this, &Control::actionGroupTriggered);

    auto actionNone = new QAction(tr("None"), this);
    actionNone->setCheckable(true);
    _actionGroup->addAction(actionNone);

    loadMetas(storages);
    const auto& allMetas = specCache().allMetas;
    auto it = allMetas.constBegin();
    while (it != allMetas.constEnd())
    {
        const auto& meta = it.value();
        auto actionDict = new QAction(meta.displayTitle(), this);
        actionDict->setCheckable(true);
        actionDict->setData(meta.name);
        _actionGroup->addAction(actionDict);
        it++;
    }
}

QMenu* Control::makeMenu(QString title, QWidget* parent)
{
    auto menu = new QMenu(title, parent);
    menu->addActions(_actionGroup->actions());
    menu->addSeparator();
    auto actnEdit = menu->addAction(tr("Edit Highlighter..."));
    auto actnNew = menu->addAction(tr("New Highlighter..."));
    connect(actnEdit, &QAction::triggered, this, &Control::editHighlighter);
    connect(actnNew, &QAction::triggered, this, &Control::newHighlighter);
    return menu;
}

void Control::showCurrent(const QString& name)
{
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

QString Control::currentHighlighter() const
{
    for (const auto& action : _actionGroup->actions())
        if (action->isChecked())
            return action->data().toString();
    return QString();
}

void Control::editHighlighter()
{
    auto& cache = specCache();
    auto name = currentHighlighter();
    if (name.isEmpty() || !cache.loadedSpecs.contains(name))
    {
        Ori::Dlg::info(tr("No highlighter is selected"));
        return;
    }

    const auto& spec = cache.loadedSpecs[name];
    if (!spec->meta.storage->readOnly())
    {
        // reload spec with code and sample text
        auto fullSpec = spec->meta.storage->loadSpec(spec->meta.source, true);
        emit editorRequested(fullSpec);
        return;
    }

    if (Ori::Dlg::yes(tr("Highlighter \"%1\" is built-in and can not be edited. "
                         "Do you want to create a new highlighter on its base instead?"
                         ).arg(spec->meta.displayTitle())))
    {
        newHighlighterWithBase(spec);
    }
}

void Control::newHighlighter()
{
    auto& cache = specCache();
    auto name = currentHighlighter();
    if (name.isEmpty() || !cache.loadedSpecs.contains(name))
    {
        QSharedPointer<Spec> spec(new Spec);
        spec->meta.storage = cache.customStorage;
        emit editorRequested(spec);
        return;
    }

    const auto& spec = cache.loadedSpecs[name];
    if (Ori::Dlg::yes(tr("Do you want to use the current highlighter \"%1\" "
                         "as a base for your new one?").arg(spec->meta.displayTitle())))
    {
        newHighlighterWithBase(spec);
        return;
    }

    QSharedPointer<Spec> newSpec(new Spec);
    newSpec->meta.storage = cache.customStorage;
    emit editorRequested(newSpec);
}

void Control::newHighlighterWithBase(const QSharedPointer<Spec>& base)
{
    auto spec = base->meta.storage->loadSpec(base->meta.source, true);
    spec->meta.name = "";
    spec->meta.source = "";
    spec->meta.title = "";
    spec->meta.storage = specCache().customStorage;
    emit editorRequested(spec);
}

} // namespace Highlighter
} // namespace Ori
