#include "SpellChecker.h"

#include "hunspell/hunspell.hxx"

SpellChecker::SpellChecker(const QString &dictionaryPath, const QString &userDictionary)
{
    _userDictionary = userDictionary;

    auto dictFile = (dictionaryPath + ".dic").toLocal8Bit();
    auto affixFile = (dictionaryPath + ".aff").toLocal8Bit();
    _hunspell = new Hunspell(dictFile.constData(), affixFile.constData());
}

SpellChecker::~SpellChecker()
{
    delete _hunspell;
}
