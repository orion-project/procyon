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

    void warning(QString msg)
    {
        qWarning() << "Highlighter" << file.fileName() << "| line" << lineNo << "|" << msg;
    }

    bool readLine()
    {
        lineNo++;
        auto line = stream.readLine().trimmed();
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

    void finalizeRule(Spec& spec, Rule& rule)
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
        spec.rules << rule;
    }

public:
    SpecLoader(QString fileName)
    {
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
            if (key == "name")
            {
                meta.name = val;
                suffice = true;
            }
            else if (key == "title")
            {
                meta.title = val;
            }
            else if (key == "rule")
            {
                return suffice;
            }
            else warning("unknown key");
        }
        return false;
    }

    void load(Spec& spec)
    {
        if (!loadMeta(spec.meta))
            return;

        Rule rule;
        rule.name = val;

        while (!stream.atEnd())
        {
            if (!readLine())
                continue;
            if (key == "rule")
            {
                finalizeRule(spec, rule);
                rule = Rule();
                rule.name = val;
            }
            else if (key == "expr")
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
            else if (key == "color")
            {
                QColor c(val);
                if (!c.isValid())
                    warning("invalid color value");
                else
                    rule.format.setForeground(c);
            }
            else if (key == "back")
            {
                QColor c(val);
                if (!c.isValid())
                    warning("invalid color value");
                else
                    rule.format.setBackground(c);
            }
            else if (key == "group")
            {
                bool ok;
                int group = val.toInt(&ok);
                if (!ok)
                    warning("invalid integer value");
                else
                    rule.group = group;
            }
            else if (key == "style")
            {
                for (const auto& style : val.split(',', Qt::SkipEmptyParts))
                {
                    auto s = style.trimmed();
                    if (s == "bold")
                        rule.format.setFontWeight(QFont::Bold);
                    else if (s == "italic")
                        rule.format.setFontItalic(true);
                    else if (s == "underline")
                        rule.format.setFontUnderline(true);
                    else if (s == "strikeout")
                        rule.format.setFontStrikeOut(true);
                    else if (s == "hyperlink")
                    {
                        rule.format.setAnchor(true);
                        rule.hyperlink = true;
                    }
                    else if (s == "multiline")
                        rule.multiline = true;
                    else warning("unknown style " + s);
                }
            }
            else if (key == "terms")
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
    return true;
}

QVector<Meta> DefaultStorage::load() const
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
            SpecLoader loader(filename);
            if (loader.loadMeta(meta))
            {
                meta.source = filename;
                metas << meta;
            }
        }
    }
    return metas;
}

Spec DefaultStorage::load(const QString& source) const
{
    Spec spec;
    SpecLoader loader(source);
    loader.load(spec);
    return spec;
}

//------------------------------------------------------------------------------
//                                 SpecCache
//------------------------------------------------------------------------------

struct SpecCache
{
    QMap<QString, Meta> allMetas;
    QMap<QString, Spec> loadedSpecs;

    const Spec& getSpec(QString name)
    {
        if (!loadedSpecs.contains(name))
        {
            const auto& meta = allMetas[name];
            Spec spec = meta.storage->load(meta.source);
            spec.meta.source = meta.source;
            spec.meta.storage = meta.storage;
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

void loadHighlighters(const QVector<QSharedPointer<SpecStorage>>& storages)
{
    for (const auto& storage : storages)
    {
        for (auto& meta : storage->load())
        {
            if (specCache().allMetas.contains(meta.name))
            {
                qWarning() << "Highlighter is already registered" << meta.name;
                continue;
            }
            meta.storage = storage;
            specCache().allMetas[meta.name] = meta;
            qDebug() << "Highlighter registered" << meta.name << meta.source;
        }
    }
}

QVector<Meta> availableHighlighters()
{
    return specCache().allMetas.values();
}

bool exists(QString name)
{
    if (specCache().allMetas.contains(name))
        return true;
    qWarning() << "Unknown highlighter" << name;
    return false;
}

//------------------------------------------------------------------------------
//                                 Highlighter
//------------------------------------------------------------------------------

Highlighter::Highlighter(QTextDocument *parent, QString name)
    : QSyntaxHighlighter(parent), _spec(specCache().getSpec(name)), _document(parent)
{
    setObjectName(name);
}

void Highlighter::highlightBlock(const QString &text)
{
    bool hasMultilines = false;
    for (const auto& rule : _spec.rules)
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
        setCurrentBlockState(0);
        for (const auto& rule : _spec.rules)
        {
            if (!rule.multiline) continue;
            offset = matchMultiline(text, rule, offset);
            if (offset < 0) break;
        }
    }
}

int Highlighter::matchMultiline(const QString &text, const Rule& rule, int initialOffset)
{
    const auto& exprBeg = rule.exprs[0];
    const auto& exprEnd = rule.exprs[1];
    QRegularExpressionMatch m;

    int start = 0;
    int offset = initialOffset;
    bool matchEnd = previousBlockState() > 0;
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
                setCurrentBlockState(1);
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

    Ori::Highlighter::loadHighlighters(storages);
    for (const auto& h : Ori::Highlighter::availableHighlighters())
    {
        auto actionDict = new QAction(h.displayTitle(), this);
        actionDict->setCheckable(true);
        actionDict->setData(h.name);
        _actionGroup->addAction(actionDict);
    }
}

QMenu* Control::makeMenu(QString title, QWidget* parent)
{
    auto menu = new QMenu(title, parent);
    menu->addActions(_actionGroup->actions());
    menu->addSeparator();
    auto actnEdit = menu->addAction(tr("Edit highlighter..."));
    auto actnNew = menu->addAction(tr("New highlighter..."));
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
    auto name = currentHighlighter();
    if (name.isEmpty())
    {
        Ori::Dlg::info(tr("No highlighter is selected"));
        return;
    }
    (new EditDialog(name))->show();
}

void Control::newHighlighter()
{
    (new EditDialog(""))->show();
}

//------------------------------------------------------------------------------
//                                 Control
//------------------------------------------------------------------------------

EditDialog::EditDialog(QString name) : QWidget()
{
    if (name.isEmpty())
        setWindowTitle(tr("Create Highlighter"));
    else
        setWindowTitle(tr("Edit Highlighter: %s").arg(name));
    setAttribute(Qt::WA_DeleteOnClose);
}

} // namespace Highlighter
} // namespace Ori
