#ifndef SPELL_CHECKER_H
#define SPELL_CHECKER_H

#ifdef ENABLE_SPELLCHECK

#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QMenu;
class QWidget;
class QTextCodec;
QT_END_NAMESPACE

class Hunspell;

class Spellchecker : public QObject
{
    Q_OBJECT

public:
    static Spellchecker* get(const QString& lang);

    ~Spellchecker();

    const QString& lang() const { return _lang; }
    bool check(const QString &word) const;
    void ignore(const QString &word);
    void save(const QString &word);
    QStringList suggest(const QString &word) const;

signals:
    void wordIgnored(const QString& word);

private:
    Spellchecker(const QString &dictFilePath, const QString &affixFilePath, const QString &userDictionaryPath);

    QString _lang;
    QString _userDictionaryPath;
    Hunspell* _hunspell = nullptr;
    QTextCodec *_codec;

    void loadUserDictionary();
};


class SpellcheckControl : public QObject
{
    Q_OBJECT

public:
    explicit SpellcheckControl(QObject* parent = nullptr);

    QMenu* makeMenu(QWidget* parent = nullptr);

    void showCurrentLang(const QString& lang);
    void setEnabled(bool on);

signals:
    void langSelected(const QString& lang);

private:
    QActionGroup* _actionGroup = nullptr;

    void actionGroupTriggered(QAction* action);
};

#endif // ENABLE_SPELLCHECK

#endif // SPELL_CHECKER_H
