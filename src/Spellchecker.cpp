#include "Spellchecker.h"

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

    // TODO: In WIN32 environment, use UTF-8 encoded paths started with the long path prefix
    _hunspell = new Hunspell(affixFilePath.toLocal8Bit().constData(),
                             dictFilePath.toLocal8Bit().constData());


/* TODO: load user dictionary

if(!_userDictionary.isEmpty()) {
        QFile userDictonaryFile(_userDictionary);
        if(userDictonaryFile.open(QIODevice::ReadOnly)) {
            QTextStream stream(&userDictonaryFile);
            for(QString word = stream.readLine(); !word.isEmpty(); word = stream.readLine())
                put_word(word);
            userDictonaryFile.close();
        } else {
            qWarning() << "User dictionary in " << _userDictionary << "could not be opened";
        }
    } else {
        qDebug() << "User dictionary not set.";
    }

*/
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
}

void Spellchecker::save(const QString &word)
{
    _hunspell->add(_codec->fromUnicode(word).toStdString());

/* TODO

if(!_userDictionary.isEmpty()) {
        QFile userDictonaryFile(_userDictionary);
        if(userDictonaryFile.open(QIODevice::Append)) {
            QTextStream stream(&userDictonaryFile);
            stream << word << "\n";
            userDictonaryFile.close();
        } else {
            qWarning() << "User dictionary in " << _userDictionary << "could not be opened for appending a new word";
        }
    } else {
        qDebug() << "User dictionary not set.";
    }

*/
}

QStringList Spellchecker::suggest(const QString &word) const
{
    QStringList variants;
    for (auto variant : _hunspell->suggest(_codec->fromUnicode(word).toStdString()))
        variants << _codec->toUnicode(QByteArray::fromStdString(variant));
    return variants;
}
