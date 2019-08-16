#include "Spellchecker.h"

#include "hunspell/hunspell.hxx"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextCodec>

#include "tools/OriSettings.h"

static QString getDictionaryDir()
{
    return qApp->applicationDirPath() + "/dicts/";
}

static QString getUserDictionaryPath(const QString& lang)
{
    QSharedPointer<QSettings> s(Ori::Settings::open());
    return QFileInfo(s->fileName()).absoluteDir().path() + "/userdict-" + lang + ".dic";
}

// detect encoding analyzing the SET option in the affix file
static QString getDictionaryEncoding(const QString& affixFilePath)
{
    QFile file(affixFilePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Unable to open affix file" << affixFilePath << file.errorString();
        return QString();
    }
    QString encoding;
    QTextStream stream(&file);
    QRegExp enc_detector("^\\s*SET\\s+([A-Z0-9\\-]+)\\s*", Qt::CaseInsensitive);
    for (QString line = stream.readLine(); !line.isEmpty(); line = stream.readLine())
        if (enc_detector.indexIn(line) > -1)
        {
            encoding = enc_detector.cap(1);
            break;
        }
    file.close();
    return encoding;
}

Spellchecker* Spellchecker::get(const QString& lang)
{
    static QMap<QString, Spellchecker*> checkers;
    if (!checkers.contains(lang))
    {
        QString dictionary = getDictionaryDir() + lang;
        QString userDictionary = getUserDictionaryPath(lang);
        auto checker = new Spellchecker(dictionary, userDictionary);
        if (checker->_hunspell)
        {
            checker->_lang = lang;
            checkers.insert(lang, checker);
        }
        else delete checker;
    }
    return checkers.contains(lang) ? checkers[lang] : nullptr;
}


Spellchecker::Spellchecker(const QString &dictionaryPath, const QString &userDictionaryPath)
{
    _userDictionaryPath = userDictionaryPath;

    QString dictFilePath = dictionaryPath + ".dic";
    QString affixFilePath = dictionaryPath + ".aff";

    QString encoding = getDictionaryEncoding(affixFilePath);
    if (encoding.isEmpty())
    {
        qWarning() << "Unable to detect dictionary encoding in affix file"
                   << affixFilePath << "Spell check is unavailable";
        return;
    }

    _codec = QTextCodec::codecForName(encoding.toLatin1().constData());
    if (!_codec)
    {
        qWarning() << "Codec not found for encoding" << encoding
                   << "detected in dictionary" << affixFilePath
                   << "Spell check is unavailable";
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
    emit dictionaryChanged();
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
    stream.setCodec("UTF-8");
    stream << word << "\n";
    file.close();
}

QStringList Spellchecker::suggest(const QString &word) const
{
    QStringList variants;
    for (auto variant : _hunspell->suggest(_codec->fromUnicode(word).toStdString()))
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
    stream.setCodec("UTF-8");
    for (QString word = stream.readLine(); !word.isEmpty(); word = stream.readLine())
        _hunspell->add(_codec->fromUnicode(word).toStdString());
    file.close();
}
