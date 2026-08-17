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

QString createTable(TableDef *table)
{
    auto res = ActionQuery(table->sqlCreate()).exec();
    if (!res.isEmpty())
    {
        QSqlDatabase::database().rollback();
        return QString("Unable to create table '%1'.\n\n%2").arg(table->tableName()).arg(res);
    }
    return QString();
}

QString addColumnIfNotExist(const QString& tableName, const QString& columnName)
{
    SelectQuery query(QString("SELECT * FROM sqlite_master WHERE type = 'table' "
                              "AND name = '%1' AND sql LIKE '%%%2%%'").arg(tableName, columnName));
    if (query.isFailed())
    {
        QSqlDatabase::database().rollback();
        return QString("Failed to check if column '%1' exists in table '%2'.\n\n%3")
                .arg(tableName, columnName, query.error());
    }

    // There should be a row containing 'CREATE TABLE' statement including the given column name
    if (query.next())
        return QString();

    auto res = ActionQuery(QString("ALTER TABLE %1 ADD COLUMN %2").arg(tableName, columnName)).exec();
    if (!res.isEmpty())
    {
        QSqlDatabase::database().rollback();
        return QString("Unable to add column '%1' into table '%2'.\n\n%3").arg(columnName, tableName, res);
    }

    return QString();
}

} // namespace Sql
} // namespace Ori
