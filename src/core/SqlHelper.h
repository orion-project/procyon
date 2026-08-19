#ifndef ORI_SQL_HELPER_H
#define ORI_SQL_HELPER_H

#include <QtSql>
#include <QString>
#include <QDebug>

namespace SqlHelper {

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


class AnyQuery
{
public:
    AnyQuery(AnyQuery& other)
    {
        // The query is not meant to be copied (because of QSqlQuery)
        // this constructor actualy takes reference to a temporary object
        // in calls like
        // auto q = AnyQuery().param().param()....exec()
        _query = std::move(other._query);
        _error = other._error;
        _record = other._record;
    }
    
    AnyQuery(const QString& sql)
    {
        _error = QStringLiteral("Query is not executed");
        _query.prepare(sql);
    }

    AnyQuery& param(const QString& name, const QVariant& value)
    {
        _query.bindValue(':' + name, value);
        return *this;
    }

    AnyQuery& exec()
    {
        if (!_query.exec())
            _error = SqlHelper::errorText(_query, true);
        else _error.clear();
        return *this;
    }

    bool next()
    {
        if (!_query.isSelect()) return false;
        bool ok =  _query.isValid() ? _query.next(): _query.first();
        if (ok) _record = _query.record();
        return ok;
    }

    bool isFailed() const { return !_error.isEmpty(); }
    const QString& error() const { return _error; }
    const QSqlRecord& record() const { return _record; }

    QString valueStr(QAnyStringView name) const { return _record.value(name).toString(); }

private:
    QString _error;
    QSqlQuery _query;
    QSqlRecord _record;
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

    QString _tableName;
};

template <class T>
QString createTable()
{
    auto res = ActionQuery(T::sqlCreate).exec();
    if (!res.isEmpty())
    {
        QSqlDatabase::database().rollback();
        return QString("Unable to create table '%1'.\n\n%2").arg(T::tableName, res);
    }
    return QString();
}

QString createTable(TableDef *table);
QString maybeAddColumn(const QString& tableName, const QString& columnName);
QString maybeAddConstrain(const QString& tableName, const QStringList& columns);
QString maybeAddIndex(const QString& tableName, const QString& columnName);

} // namespace Sql
} // namespace Ori

#endif // ORI_SQL_HELPER_H
