#include "SpellChecker.h"

#include "hunspell/hunspell.hxx"

SpellChecker::SpellChecker(const QString &dictionaryPath, const QString &userDictionary)
{
    _userDictionary = userDictionary;

    auto dictFile = dictionaryPath + ".dic";
    auto affixFile = dictionaryPath + ".aff";
    _hunspell = new Hunspell(dictFile.toLocal8Bit().constData(),
                             affixFile.toLocal8Bit().constData());
}

SpellChecker::~SpellChecker()
{
    delete _hunspell;
}
