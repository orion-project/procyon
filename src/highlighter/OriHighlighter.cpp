#include "OriHighlighter.h"

#include <QActionGroup>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QMenu>
#include <QRegularExpression>
#include <QTextDocument>

namespace Ori {
namespace Highlighter {

struct Rule
{
    QString name;
    QVector<QRegularExpression> exprs;
    QTextCharFormat format;
    int group = 0;
    bool hyperlink = false;
    int fontSizeDelta = 0;
};

struct Spec
{
    Meta meta;
    QVector<Rule> rules;
};

//------------------------------------------------------------------------------
//                                SpecLoader
//------------------------------------------------------------------------------

struct SpecLoader
{
private:
    QFile file;
    QTextStream stream;
    int lineNo = 0;
    QString key, val;

    void error(QString msg)
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
            error("key not found");
            return false;
        }
        key = line.first(keyLen).trimmed();
        val = line.sliced(keyLen+1).trimmed();
        //qDebug() << "Highlighter" << file.fileName() << "| line" << lineNo << "| key" << key << "| value" << val;
        return true;
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

        meta.file = file.fileName();

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
            else error("unknown key");
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
                spec.rules << rule;
                rule = Rule();
                rule.name = val;
            }
            else if (key == "expr")
            {
                QRegularExpression expr(val);
                if (!expr.isValid())
                    error("invalid expression");
                else
                    rule.exprs << expr;
            }
            else if (key == "color")
            {
                QColor c(val);
                if (!c.isValid())
                    error("invalid color value");
                else
                    rule.format.setForeground(c);
            }
            else if (key == "back")
            {
                QColor c(val);
                if (!c.isValid())
                    error("invalid color value");
                else
                    rule.format.setBackground(c);
            }
            else if (key == "group")
            {
                bool ok;
                int group = val.toInt(&ok);
                if (!ok)
                    error("invalid integer value");
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
                    else error("unknown style " + s);
                }
            }
            else error("unknown key");
        }
        spec.rules << rule;
    }
};

//------------------------------------------------------------------------------
//                                    SpecCache
//------------------------------------------------------------------------------

struct SpecCache
{
    QMap<QString, Meta> allMetas;
    QMap<QString, Spec> loadedSpecs;

    const Spec& getSpec(QString name)
    {
        if (!loadedSpecs.contains(name))
        {
            Spec spec;
            SpecLoader loader(allMetas[name].file);
            loader.load(spec);
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

void loadHighlighters()
{
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
        return;
    }
    qDebug() << "Hightlighters dir" << dir.path();
    for (auto& fileInfo : dir.entryInfoList())
        if (fileInfo.fileName().endsWith(".phl"))
        {
            Meta meta;
            SpecLoader loader(fileInfo.absoluteFilePath());
            if (loader.loadMeta(meta))
            {
                specCache().allMetas[meta.name] = meta;
                qDebug() << "Highlighter loaded" << meta.name << meta.file;
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
    for (const auto& rule : _spec.rules)
    {
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
}

//------------------------------------------------------------------------------
//                                SpecLoader
//------------------------------------------------------------------------------

Control::Control(QObject *parent) : QObject(parent)
{
    _actionGroup = new QActionGroup(parent);
    _actionGroup->setExclusive(true);
    connect(_actionGroup, &QActionGroup::triggered, this, &Control::actionGroupTriggered);

    auto actionNone = new QAction(tr("None"), this);
    actionNone->setCheckable(true);
    _actionGroup->addAction(actionNone);

    Ori::Highlighter::loadHighlighters();
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
    if (!_actionGroup) return nullptr;

    auto menu = new QMenu(title, parent);
    menu->addActions(_actionGroup->actions());
    return menu;
}

void Control::showCurrent(const QString& name)
{
    if (!_actionGroup) return;

    for (auto action : _actionGroup->actions())
        if (action->data().toString() == name)
        {
            action->setChecked(true);
            break;
        }
}

void Control::setEnabled(bool on)
{
    if (_actionGroup) _actionGroup->setEnabled(on);
}

void Control::actionGroupTriggered(QAction* action)
{
    emit selected(action->data().toString());
}

} // namespace Highlighter
} // namespace Ori
