#include "SpellChecker.h"

#include "hunspell/hunspell.hxx"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QTextCodec>

static QString getDictionaryDir()
{
    return qApp->applicationDirPath() + "/dicts/";
}

static QString getUserDictionaryPath(const QString& lang)
{
    return QString(); // TODO
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

SpellChecker* SpellChecker::get(const QString& lang)
{
    static QMap<QString, SpellChecker*> checkers;
    if (!checkers.contains(lang))
    {
        QString dictionary = getDictionaryDir() + lang;
        QString userDictionary = getUserDictionaryPath(lang);
        auto checker = new SpellChecker(dictionary, userDictionary);
        if (checker->_hunspell)
            checkers.insert(lang, checker);
        else
            delete checker;
    }
    return checkers.contains(lang) ? checkers[lang] : nullptr;
}


SpellChecker::SpellChecker(const QString &dictionaryPath, const QString &userDictionaryPath)
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

    // TODO: load user dictionary

    // TODO: In WIN32 environment, use UTF-8 encoded paths started with the long path prefix
    _hunspell = new Hunspell(affixFilePath.toLocal8Bit().constData(),
                             dictFilePath.toLocal8Bit().constData());
}

SpellChecker::~SpellChecker()
{
    if (_hunspell) delete _hunspell;
}

bool SpellChecker::check(const QString &word) const
{
    return _hunspell->spell(_codec->fromUnicode(word).toStdString());
}
