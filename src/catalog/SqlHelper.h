#ifndef SQLHELPER_H
#define SQLHELPER_H

#include <QtSql>
#include <QString>
#include <QDebug>

//-----------------------------------------------------------------------------------------------

namespace SqlHelper {

void addField(QSqlRecord &record, const QString &name, QVariant::Type type, const QVariant &value);
void addField(QSqlRecord &record, const QString &name, const QVariant &value);

QString errorText(const QSqlQuery &query, bool includeSql = false);
QString errorText(const QSqlQuery *query, bool includeSql = false);
QString errorText(const QSqlTableModel &model);
QString errorText(const QSqlTableModel *model);
QString errorText(const QSqlError &error);

} // namespace SqlHelper

namespace Ori {
namespace Sql {

class ActionQuery
{
public:
    ActionQuery(const QString& sql)
    {
        _query.prepare(sql);
    }

    ActionQuery& param(const QString& name, const QVariant& value)
    {
        _query.bindValue(':' + name, value);
        return *this;
    }

    QString exec()
    {
        if (!_query.exec())
            return SqlHelper::errorText(_query, true);
        return QString();
    }

private:
    QSqlQuery _query;
};


class SelectQuery
{
public:
    SelectQuery(const QString& sql)
    {
        if (!_query.exec(sql))
            _error = SqlHelper::errorText(_query, true);
    }

    bool isFailed() const { return !_error.isEmpty(); }
    const QString& error() const { return _error; }
    const QSqlRecord& record() const { return _record; }

    bool next()
    {
        if (!_query.isSelect()) return false;
        bool ok =  _query.isValid() ? _query.next(): _query.first();
        if (ok) _record = _query.record();
        return ok;
    }

protected:
    QSqlQuery _query;
    QSqlRecord _record;

private:
    QString _error;
};


class TableDef
{
public:
    virtual ~TableDef();

    const QString& tableName() const { return _tableName; }

    virtual QString sqlCreate() const = 0;

    virtual QString sqlSelectAll() const {
        return QString("SELECT * FROM %1").arg(_tableName);
    }

    virtual QString sqlCountAll() {
        return QString("SELECT COUNT(Id) FROM %1").arg(_tableName);
    }

    virtual QString sqlSelectById(int id) const {
        return QString("SELECT * FROM %1 WHERE Id = %2").arg(_tableName).arg(id);
    }

    virtual QString sqlSelectById(const QString& id) const {
        return QString("SELECT * FROM %1 WHERE Id = '%2'").arg(_tableName).arg(id);
    }

    virtual QString sqlSelectMaxId() const {
        return QString("SELECT MAX(Id) FROM %1").arg(_tableName);
    }

    virtual QString sqlCheckId(int id) const {
        return QString("SELECT Id FROM %1 WHERE Id = %2 LIMIT 1").arg(_tableName).arg(id);
    }

    virtual QString sqlCheckId(const QString& id) const {
        return QString("SELECT Id FROM %1 WHERE Id = '%2' LIMIT 1").arg(_tableName).arg(id);
    }

protected:
    TableDef(const QString& tableName): _tableName(tableName) {}

private:
    QString _tableName;
};

} // namespace Sql
} // namespace Ori

#endif // SQLHELPER_H
