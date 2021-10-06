#include "Spellchecker.h"

#ifdef ENABLE_SPELLCHECK

#include "hunspell/hunspell.hxx"

#include <QActionGroup>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMenu>
#include <QRegularExpression>
#include <QTextCodec>

#include "tools/OriSettings.h"

QMap<QString, QString> langNamesMap();

//------------------------------------------------------------------------------
//                                Spellchecker
//------------------------------------------------------------------------------

namespace  {
static const QString dictFileExt(".dic");
static const QString affixFileExt(".aff");
}

static QDir dictionaryDir()
{
    QDir dir(qApp->applicationDirPath() + "/dicts");

#ifdef Q_OS_MAC
    if (!dir.exists())
    {
        // Look near the application bundle, it is for development mode
        return QDir(qApp->applicationDirPath() % "/../../../dicts");
    }
#endif

    return dir;
}

static QStringList dictionaries()
{
    QStringList dicts;

    for (auto& fileInfo : dictionaryDir().entryInfoList())
        if (fileInfo.fileName().endsWith(dictFileExt))
            dicts << fileInfo.baseName();

    return dicts;
}

static QString userDictionaryPath(const QString& lang)
{
    QSharedPointer<QSettings> s(Ori::Settings::open());
    QDir dictDir = QFileInfo(s->fileName()).absoluteDir();
    QFileInfo dictFile(dictDir, "userdict-" + lang + dictFileExt);
    return dictFile.absoluteFilePath();
}

// detect encoding analyzing the SET option in the affix file
static QString dictionaryEncoding(const QString& affixFilePath)
{
    QFile file(affixFilePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Unable to open affix file" << affixFilePath << file.errorString();
        return QString();
    }
    QString encoding;
    QTextStream stream(&file);
    QRegularExpression enc_detector("^\\s*SET\\s+([A-Z0-9\\-]+)\\s*", QRegularExpression::CaseInsensitiveOption);
    for (QString line = stream.readLine(); !line.isEmpty(); line = stream.readLine())
    {
        auto m = enc_detector.match(line);
        if (m.hasMatch())
        {
            encoding = m.captured(1);
            break;
        }
    }
    file.close();
    return encoding;
}

Spellchecker* Spellchecker::get(const QString& lang)
{
    if (lang.isEmpty()) return nullptr;

    static QMap<QString, Spellchecker*> checkers;

    if (!checkers.contains(lang))
    {
        QDir dictDir = dictionaryDir();

        QFileInfo dictFile(dictDir, lang + dictFileExt);
        if (!dictFile.exists())
        {
            qWarning() << "Dictionary file does not exist" << dictFile.filePath();
            return nullptr;
        }

        QFileInfo affixFile(dictDir, lang + affixFileExt);
        if (!affixFile.exists())
        {
            qWarning() << "Affix file does not exist" << affixFile.filePath();
            return nullptr;
        }

        auto checker = new Spellchecker(dictFile.absoluteFilePath(),
                                        affixFile.absoluteFilePath(),
                                        userDictionaryPath(lang));
        if (!checker->_hunspell)
        {
            delete checker;
            return nullptr;
        }

        checker->_lang = lang;
        checkers.insert(lang, checker);
    }

    return checkers[lang];
}


Spellchecker::Spellchecker(const QString &dictFilePath, const QString& affixFilePath, const QString &userDictionaryPath)
{
    _userDictionaryPath = userDictionaryPath;

    QString encoding = dictionaryEncoding(affixFilePath);
    if (encoding.isEmpty())
    {
        qWarning() << "Unable to detect dictionary encoding in affix file"
                   << affixFilePath << "Spellcheck is unavailable";
        return;
    }

    _codec = QTextCodec::codecForName(encoding.toLatin1().constData());
    if (!_codec)
    {
        qWarning() << "Codec not found for encoding" << encoding
                   << "detected in dictionary" << affixFilePath
                   << "Spellcheck is unavailable";
        return;
    }

    _hunspell = new Hunspell(affixFilePath.toLocal8Bit().constData(),
                             dictFilePath.toLocal8Bit().constData());

    loadUserDictionary();
}

Spellchecker::~Spellchecker()
{
    if (_hunspell) delete _hunspell;
}

bool Spellchecker::check(const QString &word) const
{
    return _hunspell->spell(_codec->fromUnicode(word).toStdString());
}

void Spellchecker::ignore(const QString &word)
{
    _hunspell->add(_codec->fromUnicode(word).toStdString());
    emit wordIgnored(word);
}

void Spellchecker::save(const QString &word)
{
    if (_userDictionaryPath.isEmpty()) return;

    QFile file(_userDictionaryPath);
    if (!file.open(QIODevice::Append))
    {
        qWarning() << "Unable to open user dictionary file for writing"
                   << _userDictionaryPath << file.errorString();
        return;
    }

    QTextStream stream(&file);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
    stream << word << "\n";
    file.close();
}

QStringList Spellchecker::suggest(const QString &word) const
{
    QStringList variants;
    for (auto& variant : _hunspell->suggest(_codec->fromUnicode(word).toStdString()))
        variants << _codec->toUnicode(QByteArray::fromStdString(variant));
    return variants;
}

void Spellchecker::loadUserDictionary()
{
    if (_userDictionaryPath.isEmpty()) return;

    QFile file(_userDictionaryPath);
    if (!file.exists()) return;
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Unable to open user dictionary file for reading"
                   << _userDictionaryPath << file.errorString();
        return;
    }

    QTextStream stream(&file);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    stream.setEncoding(QStringConverter::Utf8);
#else
    stream.setCodec("UTF-8");
#endif
    for (QString word = stream.readLine(); !word.isEmpty(); word = stream.readLine())
        _hunspell->add(_codec->fromUnicode(word).toStdString());
    file.close();
}

//------------------------------------------------------------------------------
//                            SpellcheckerControl
//------------------------------------------------------------------------------

SpellcheckControl::SpellcheckControl(QObject* parent) : QObject(parent)
{
    auto dicts = dictionaries();
    if (dicts.isEmpty()) return;

    _actionGroup = new QActionGroup(parent);
    _actionGroup->setExclusive(true);
    connect(_actionGroup, &QActionGroup::triggered, this, &SpellcheckControl::actionGroupTriggered);

    auto actionNone = new QAction(tr("None"), this);
    actionNone->setCheckable(true);
    _actionGroup->addAction(actionNone);

    auto langNames = langNamesMap();
    for (auto& lang : dicts)
    {
        auto langName = langNames.contains(lang) ? langNames[lang] : lang;
        auto actionDict = new QAction(langName, this);
        actionDict->setCheckable(true);
        actionDict->setData(lang);
        _actionGroup->addAction(actionDict);
    }
}

QMenu* SpellcheckControl::makeMenu(QWidget* parent)
{
    if (!_actionGroup) return nullptr;

    auto menu = new QMenu(tr("Spellcheck"), parent);
    menu->addActions(_actionGroup->actions());
    return menu;
}

void SpellcheckControl::showCurrentLang(const QString& lang)
{
    if (!_actionGroup) return;

    for (auto action : _actionGroup->actions())
        if (action->data().toString() == lang)
        {
            action->setChecked(true);
            break;
        }
}

void SpellcheckControl::setEnabled(bool on)
{
    if (_actionGroup) _actionGroup->setEnabled(on);
}

void SpellcheckControl::actionGroupTriggered(QAction* action)
{
    emit langSelected(action->data().toString());
}

#endif // ENABLE_SPELLCHECK
