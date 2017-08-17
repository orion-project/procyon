#ifndef GLASS_H
#define GLASS_H

#include <QString>
#include <QMap>

class DispersionFormula;

class Glass
{
public:
    virtual ~Glass() {}

    int id() const { return _id; }
    const QString& title() const { return _title; }
    const QString& comment() const { return _comment; }
    double lambdaMin() const { return _lambdaMin; }
    double lambdaMax() const { return _lambdaMax; }
    DispersionFormula* formula() const { return _formula; }
    const QMap<QString, double>& coeffValues() const { return _coeffValues; }
    virtual QString prepare() { return QString(); }
    virtual double calcIndex(const double& lambda) const = 0;
//    virtual double calcIndex1(const double& lambda) const = 0;
//    virtual double calcIndex2(const double& lambda) const = 0;

    void assign(Glass* g);

protected:
    Glass(DispersionFormula* formula) : _formula(formula) {}

    QString getCoeff(const QString& name, double* value) const;
    QString getCoeffs(const QStringList& names, std::initializer_list<double*> values) const;

private:
    int _id;
    QString _title, _comment;
    DispersionFormula* _formula;
    double _lambdaMin, _lambdaMax;
    QMap<QString, double> _coeffValues;

    friend class Catalog;
    friend class GlassEditor;
    friend class GlassManager;
};


class GlassShott : public Glass
{
public:
    QString prepare() override;
    double calcIndex(const double& lambda) const override;
//    double calcIndex1(const double& lambda) const override { return 0; }
//    double calcIndex2(const double& lambda) const override { return 0; }

private:
    GlassShott(DispersionFormula* formula) : Glass(formula) {}
    friend class ShottFormula;

    double c1, c2, c3, c4, c5, c6;
};


class GlassSellmeier : public Glass
{
public:
    QString prepare() override;
    double calcIndex(const double& lambda) const override;
//    double calcIndex1(const double& lambda) const override { return 0; }
//    double calcIndex2(const double& lambda) const override { return 0; }

private:
    GlassSellmeier(DispersionFormula* formula) : Glass(formula) {}
    friend class SellmeierFormula;

    double b1, b2, b3, c1, c2, c3;
};


class GlassReznik : public Glass
{
public:
    QString prepare() override;
    double calcIndex(const double& lambda) const override;
//    double calcIndex1(const double& lambda) const override { return 0; }
//    double calcIndex2(const double& lambda) const override { return 0; }

private:
    GlassReznik(DispersionFormula* formula) : Glass(formula) {}
    friend class ReznikFormula;

    double c1, c2, c3, c4, c5, c6, c7, c8, c9, c10, c11;
    double L_av, delta_L, lambda_av, delta_lambda;
};


class GlassCustom : public Glass
{
public:
    double calcIndex(const double& lambda) const override { return lambda; }
//    double calcIndex1(const double& lambda) const override { return 0; }
//    double calcIndex2(const double& lambda) const override { return 0; }

private:
    GlassCustom(DispersionFormula* formula) : Glass(formula) {}
    friend class CustomFormula;
};

#endif // GLASS_H

