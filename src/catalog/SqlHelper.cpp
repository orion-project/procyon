#include "SqlHelper.h"

namespace SqlHelper {

void addField(QSqlRecord &record, const QString &name, QVariant::Type type, const QVariant &value)
{
    QSqlField field(name, type);
    field.setValue(value);
    record.append(field);
}

void addField(QSqlRecord &record, const QString &name, const QVariant &value)
{
    QSqlField field(name, value.type());
    field.setValue(value);
    record.append(field);
}

QString errorText(const QSqlQuery &query, bool includeSql)
{
    return errorText(&query, includeSql);
}

QString errorText(const QSqlQuery *query, bool includeSql)
{
    QString text;
    if (includeSql)
        text = query->lastQuery() + "\n\n";
    return text + errorText(query->lastError());
}

QString errorText(const QSqlTableModel &model)
{
    return errorText(model.lastError());
}

QString errorText(const QSqlTableModel *model)
{
    return errorText(model->lastError());
}

QString errorText(const QSqlError &error)
{
    return QString("%1\n%2").arg(error.driverText()).arg(error.databaseText());
}

} // namespace SqlHelper

namespace Ori {
namespace Sql {

TableDef::~TableDef()
{}

} // namespace Sql
} // namespace Ori
