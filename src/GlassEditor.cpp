#include "Appearance.h"
#include "Catalog.h"
#include "Glass.h"
#include "GlassEditor.h"
#include "helpers/OriDialogs.h"
#include "helpers/OriLayouts.h"
#include "widgets/OriValueEdit.h"
#include "qwt-mml-dev/formulaview.h"

#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>

bool GlassEditor::createGlass(Catalog* catalog, FolderItem* parent)
{
    GlassEditor editor(CreateGlass, catalog);
    editor._parentFolder = parent;
    editor.formulaSelected();
    return editor.run();
}

bool GlassEditor::editGlass(Catalog *catalog, GlassItem* item)
{
    GlassEditor editor(EditGlass, catalog);
    if (!editor.populate(item)) return false;
    return editor.run();
}

GlassEditor::GlassEditor(DialogMode mode, Catalog *catalog) :
    Ori::Dlg::BasicConfigDialog(qApp->activeWindow()), _mode(mode), _catalog(catalog)
{
    setWindowTitle(tr("Material Properties"));
    setWindowIcon(QIcon(":/icon/main"));
    setObjectName("GlassEditor");

    createPages({createGeneralPage(), createFormulaPage(), createCoeffsPage(), createCommentPage()});
}

QWidget* GlassEditor::createGeneralPage()
{
    auto page = new Ori::Dlg::BasicConfigPage(tr("General"), ":/icon/glass_blue");
    page->setLongTitle(tr("General Properties"));

    _titleEditor = new QLineEdit;
    _lambdaMinEditor = new Ori::Widgets::ValueEdit;
    _lambdaMaxEditor = new Ori::Widgets::ValueEdit;

    Z::Gui::setValueFont(_titleEditor);
    Z::Gui::setValueFont(_lambdaMinEditor);
    Z::Gui::setValueFont(_lambdaMaxEditor);

    auto layout = new QFormLayout;
    layout->addRow(new QLabel(tr("Material name")), _titleEditor);
    layout->addRow(new QLabel(tr("Min wavelength, <b>μm</b>")), _lambdaMinEditor);
    layout->addRow(new QLabel(tr("Max wavelength, <b>μm</b>")), _lambdaMaxEditor);

    auto frame = new QFrame;
    frame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    frame->setLayout(layout);

    page->add({frame});
    return page;
}

QWidget* GlassEditor::createFormulaPage()
{
    auto page = new Ori::Dlg::BasicConfigPage(tr("Formula"), ":/icon/formula");
    page->setLongTitle(tr("Dispersion Formula"));

    _formulaSelector = new QComboBox;
    for (DispersionFormula* formula: dispersionFormulas().values())
        _formulaSelector->addItem(formula->icon(), tr(formula->name()), QString(formula->name()));
    connect(_formulaSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(formulaSelected()));

    _formulaView = new FormulaView();
    _formulaView->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    _formulaView->setFontSize(72);
    _formulaView->setPaddings(6);
    _formulaView->setTransformation(true);
    _formulaView->setScale(true);

    page->add({_formulaSelector, _formulaView});
    return page;
}

QWidget* GlassEditor::createCoeffsPage()
{
    auto page = new Ori::Dlg::BasicConfigPage(tr("Coefficients"), ":/icon/coeffs");
    page->setLongTitle(tr("Formula Coefficients"));

    _coeffsLayout = new QFormLayout;

    auto frame = new QFrame;
    frame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    frame->setLayout(_coeffsLayout);

    page->add({frame});
    return page;
}

QWidget* GlassEditor::createCommentPage()
{
    auto page = new Ori::Dlg::BasicConfigPage(tr("Comment"), ":/icon/comment");

    _commentEditor = new QTextEdit;
    _commentEditor->setAcceptRichText(false);
    Z::Gui::setValueFont(_commentEditor);

    page->add({_commentEditor});
    return page;
}

bool GlassEditor::populate(GlassItem* item)
{
    if (!item->glass())
    {
        QString res = _catalog->loadGlass(item);
        if (!res.isEmpty())
        {
            Ori::Dlg::error(res);
            return false;
        }
    }

    _glassItem = item;
    Glass* glass = item->glass();
    _titleEditor->setText(glass->title());
    _lambdaMinEditor->setValue(glass->lambdaMin());
    _lambdaMaxEditor->setValue(glass->lambdaMax());
    _commentEditor->setPlainText(glass->comment());

    QString formulaName(_glassItem->formula()->name());
    int currentIndex = _formulaSelector->currentIndex();
    for (int i = 0; i < _formulaSelector->count(); i++)
        if (_formulaSelector->itemData(i).toString() == formulaName)
        {
            if (i == currentIndex) formulaSelected();
            else _formulaSelector->setCurrentIndex(i);
            break;
        }

    for (const QString& name : _glassItem->formula()->coeffNames())
        if (_coeffEditors.contains(name))
            _coeffEditors[name]->setValue(glass->coeffValues()[name]);

    return true;
}

bool GlassEditor::collect()
{
    if (glassTitle().isEmpty())
    {
        Ori::Dlg::warning(tr("Material name can not be empty."));
        return false;
    }

    Glass *glass = formula()->makeGlass();
    if (_mode == EditGlass)
        glass->assign(_glassItem->glass());

    glass->_title = glassTitle();
    glass->_lambdaMin = lambdaMin();
    glass->_lambdaMax = lambdaMax();
    glass->_comment = glassComment();
    glass->_coeffValues.clear();
    for (const QString& name : _coeffEditors.keys())
        glass->_coeffValues[name] = _coeffEditors[name]->value();

    auto res = _mode == CreateGlass
            ? _catalog->createGlass(_parentFolder, glass)
            : _catalog->updateGlass(_glassItem, glass);

    if (!res.isEmpty())
    {
        Ori::Dlg::error(res);
        return false;
    }

    return true;
}

QString GlassEditor::glassTitle() const { return _titleEditor->text().trimmed(); }
QString GlassEditor::glassComment() const  { return _commentEditor->toPlainText(); }
double GlassEditor::lambdaMin() const { return _lambdaMinEditor->value(); }
double GlassEditor::lambdaMax() const { return _lambdaMaxEditor->value(); }
DispersionFormula* GlassEditor::formula() const { return dispersionFormulas()[_formulaSelector->currentData().toString()]; }

void GlassEditor::formulaSelected()
{
    _formulaView->loadFormula(QString(":/formula/%1").arg(formula()->name()));
    updateCoeffEditors();
}

void GlassEditor::updateCoeffEditors()
{
    for (const QString& name : _coeffEditors.keys())
        _coeffsBackup[name] = _coeffEditors[name]->value();
    qDeleteAll(_coeffLabels.values()); _coeffLabels.clear();
    qDeleteAll(_coeffEditors.values()); _coeffEditors.clear();
    QStringList coeffs = formula()->coeffNames();
    for (const QString& name : coeffs)
    {
        int subIndex = name.indexOf('_');
        QString labelName = subIndex > 0
            ? QString("%1<sub>%2</sub> =").arg(name.left(subIndex))
                .arg(name.right(name.length() - subIndex - 1))
            : QString("%1 =").arg(name);
        auto label = Z::Gui::makeSymbolLabel(labelName);
        auto editor = new Ori::Widgets::ValueEdit;
        Z::Gui::setValueFont(editor);
        _coeffsLayout->addRow(label, editor);
        _coeffEditors.insert(name, editor);
        _coeffLabels.insert(name, label);
    }
    for (const QString& name : coeffs)
        _coeffEditors[name]->setValue(_coeffsBackup[name]);
}
