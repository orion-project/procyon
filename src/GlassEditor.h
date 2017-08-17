#ifndef GLASSEDITOR_H
#define GLASSEDITOR_H

#include "dialogs/OriBasicConfigDlg.h"

#include <QMap>

QT_BEGIN_NAMESPACE
class QComboBox;
class QFormLayout;
class QLabel;
class QLineEdit;
class QTextEdit;
QT_END_NAMESPACE

class Catalog;
class GlassItem;
class FolderItem;
class DispersionFormula;

class FormulaView;

namespace Ori {
namespace Widgets {
class ValueEdit;
}}

class GlassEditor : public Ori::Dlg::BasicConfigDialog
{
    Q_OBJECT

public:
    static bool createGlass(Catalog *catalog, FolderItem* parent);
    static bool editGlass(Catalog *catalog, GlassItem* item);

private slots:
    void formulaSelected();

private:
    enum DialogMode { CreateGlass, EditGlass };

    explicit GlassEditor(DialogMode mode, Catalog* catalog);

    QLineEdit *_titleEditor;
    QComboBox *_formulaSelector;
    Ori::Widgets::ValueEdit *_lambdaMinEditor, *_lambdaMaxEditor;
    QTextEdit* _commentEditor;
    FormulaView* _formulaView;
    QFormLayout* _coeffsLayout;
    QMap<QString, Ori::Widgets::ValueEdit*> _coeffEditors;
    QMap<QString, QLabel*> _coeffLabels;
    QMap<QString, double> _coeffsBackup;

    DialogMode _mode;
    Catalog *_catalog;
    GlassItem *_glassItem = nullptr;
    FolderItem *_parentFolder;

    bool populate(GlassItem* item);
    bool collect() override;

    DispersionFormula* formula() const;
    QString glassTitle() const;
    QString glassComment() const;
    double lambdaMin() const;
    double lambdaMax() const;

    QWidget* createGeneralPage();
    QWidget* createFormulaPage();
    QWidget* createCoeffsPage();
    QWidget* createCommentPage();

    void updateCoeffEditors();
};

#endif // GLASSEDITOR_H

