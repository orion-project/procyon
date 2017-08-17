#include "Glass.h"
#include "Catalog.h" // TODO extract DispersionFormula definition into separate h-file

#include <QApplication>
#include <QDebug>

#include <cmath>
using namespace std;

#define DEBUG_GLASS_CALC

// TODO check if strings from this file are put into translation

void Glass::assign(Glass* g)
{
    _id = g->_id;
    _title = g->_title;
    _comment = g->_comment;
    _lambdaMin = g->_lambdaMin;
    _lambdaMax = g->_lambdaMax;
}

QString Glass::getCoeff(const QString& name, double* value) const
{
    if (!_coeffValues.contains(name))
        return qApp->tr("Formula coefficient '%1' is not set.").arg(name);
    *value = _coeffValues[name];
    return QString();
}

QString Glass::getCoeffs(const QStringList& names, std::initializer_list<double*> values) const
{
    Q_ASSERT(names.size() == values.size());
    int nameIndex = 0;
    for (auto value = values.begin(); value != values.end(); value++)
    {
        QString res = getCoeff(names.at(nameIndex), *value);
        if (!res.isEmpty()) return res;
        nameIndex++;
    }
    return QString();
}

//------------------------------------------------------------------------------

QString GlassShott::prepare()
{
    return getCoeffs(formula()->coeffNames(), {&c1, &c2, &c3, &c4, &c5, &c6});
}

double GlassShott::calcIndex(const double& lambda) const
{
    const double l2 = lambda * lambda;
    return sqrt(c1 + c2*l2 + c3/l2 + c4/l2/l2 + c5/l2/l2/l2 + c6/l2/l2/l2/l2);
}

//------------------------------------------------------------------------------

QString GlassSellmeier::prepare()
{
    return getCoeffs(formula()->coeffNames(), {&b1, &b2, &b3, &c1, &c2, &c3});
}

double GlassSellmeier::calcIndex(const double& lambda) const
{
    const double l2 = lambda * lambda;
    return sqrt(1 + b1*l2/(l2 - c1) + b2*l2/(l2 - c2) + b3*l2/(l2 - c3));
}

//------------------------------------------------------------------------------

QString GlassReznik::prepare()
{
    QString res = getCoeffs(formula()->coeffNames(), {&c1, &c2, &c3, &c4, &c5, &c6, &c7, &c8, &c9, &c10, &c11});
    if (!res.isEmpty()) return res;
    const double lambda_min_2 = lambdaMin() * lambdaMin();
    const double lambda_max_2 = lambdaMax() * lambdaMax();
    const double L_min = 1.0 / (lambda_min_2 - c1);
    const double L_max = 1.0 / (lambda_max_2 - c1);
    L_av = (L_max + L_min) / 2.0;
    delta_L = (L_min - L_max) / 2.0; // TODO check: really (min-max)? not (max-min)?
    lambda_av = (lambda_max_2 + lambda_min_2) / 2.0;
    delta_lambda = (lambda_max_2 - lambda_min_2) / 2.0;

#ifdef DEBUG_GLASS_CALC
    qDebug() << "lambda_min_2:" << lambda_min_2 << "| lambda_max_2:" << lambda_max_2
             << "| L_min:" << L_min << "| L_max:" << L_max << "| L_av:" << L_av << "| delta_L:" << delta_L
             << "| lambda_av:" << lambda_av << "| delta_lambda:" << delta_lambda;
#endif

    return QString();
}

double GlassReznik::calcIndex(const double& lambda) const
{
    const double l2 = lambda * lambda;
    const double L_lambda = 1.0 / (l2 - c1);
    const double m = (L_lambda - L_av) / delta_L;
    const double n = (l2 - lambda_av) / delta_lambda;
    return c2 + c4 * n + c6 * n*n + c8 * n*n*n + c10 * n*n*n*n +
                c3 * m + c5 * m*m + c7 * m*m*m + c9  * m*m*m*m + c11 * m*m*m*m*m;
}
