#ifndef SPELL_CHECKER_H
#define SPELL_CHECKER_H

#include <QString>

class Hunspell;

class SpellChecker
{
public:
    SpellChecker(const QString &dictionaryPath, const QString &userDictionary);
    ~SpellChecker();

private:
    Hunspell* _hunspell;
    QString _userDictionary;
};

#endif // SPELL_CHECKER_H
