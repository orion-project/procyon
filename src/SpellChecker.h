#ifndef SPELL_CHECKER_H
#define SPELL_CHECKER_H

#include <QString>

class Hunspell;

class SpellChecker
{
public:
    static SpellChecker* get(const QString& lang);

    ~SpellChecker();

    bool check(const QString &word) const;

private:
    SpellChecker(const QString &dictionaryPath, const QString &userDictionaryPath);

    QString _userDictionaryPath;
    Hunspell* _hunspell = nullptr;
    QTextCodec *_codec;
};

#endif // SPELL_CHECKER_H
