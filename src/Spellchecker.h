#ifndef SPELL_CHECKER_H
#define SPELL_CHECKER_H

#include <QString>

class Hunspell;

class Spellchecker
{
public:
    static Spellchecker* get(const QString& lang);

    ~Spellchecker();

    bool check(const QString &word) const;

private:
    Spellchecker(const QString &dictionaryPath, const QString &userDictionaryPath);

    QString _userDictionaryPath;
    Hunspell* _hunspell = nullptr;
    QTextCodec *_codec;
};

#endif // SPELL_CHECKER_H
