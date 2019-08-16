#ifndef SPELL_CHECKER_H
#define SPELL_CHECKER_H

#include <QString>

class Hunspell;

class Spellchecker
{
public:
    static Spellchecker* get(const QString& lang);

    ~Spellchecker();

    const QString& lang() const { return _lang; }
    bool check(const QString &word) const;
    void ignore(const QString &word);
    void save(const QString &word);
    QStringList suggest(const QString &word) const;

private:
    Spellchecker(const QString &dictionaryPath, const QString &userDictionaryPath);

    QString _lang;
    QString _userDictionaryPath;
    Hunspell* _hunspell = nullptr;
    QTextCodec *_codec;
};

#endif // SPELL_CHECKER_H
