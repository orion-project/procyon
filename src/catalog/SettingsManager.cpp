#include "SettingsManager.h"

#include "SqlHelper.h"

using namespace Ori::Sql;

//------------------------------------------------------------------------------
//                              SettingsTableDef
//------------------------------------------------------------------------------

namespace  {

class SettingsTableDef : public TableDef
{
public:
    SettingsTableDef() : TableDef("Settings") {}

    const QString id = "Id";
    const QString value = "Value";

    QString sqlCreate() const override {
        return "CREATE TABLE IF NOT EXISTS Settings (Id, Value)";
    }

    const QString sqlInsert = "INSERT INTO Settings (Id, Value) VALUES (:Id, :Value)";
    const QString sqlUpdate = "UPDATE Settings SET Value = :Value WHERE Id = :Id";
};

SettingsTableDef* settingsTable() { static SettingsTableDef t; return &t; }

} // namespace

//------------------------------------------------------------------------------
//                              SettingsManager
//------------------------------------------------------------------------------

QString SettingsManager::prepare()
{
    return createTable(settingsTable());
}

QMap<QString, QVariant> SettingsManager::readSettings(const QString& idPattern) const
{
    auto table = settingsTable();

    QMap<QString, QVariant> values;
    SelectQuery query(QString("SELECT id, value FROM %1 WHERE id LIKE '%2'").arg(table->tableName(), idPattern));
    if (query.isFailed())
    {
        qWarning() << "Unable to select setting" << idPattern << query.error();
        return values;
    }
    while (query.next())
    {
        auto r = query.record();
        values[r.value(0).toString()] = r.value(1);
    }
    return values;
}

QString SettingsManager::remove(const QString& id)
{
    auto table = settingsTable();
    auto res = ActionQuery(QString("DELETE FROM %1 WHERE id = '%2'").arg(table->tableName(), id)).exec();
    if (!res.isEmpty())
    {
        qWarning() << "Error while delete setting" << id << res;
        return res;
    }
    return QString();
}

QString SettingsManager::writeValue(const QString& id, const QVariant& value) const
{
    auto table = settingsTable();

    SelectQuery query(table->sqlCheckId(id));
    if (query.isFailed())
    {
        qWarning() << "Unable to write setting" << id << query.error();
        return query.error();
    }
    QString sql = query.next() ? table->sqlUpdate : table->sqlInsert;
    QString res = ActionQuery(sql)
            .param(table->id, id)
            .param(table->value, value)
            .exec();
    if (!res.isEmpty())
    {
        qWarning() << "Error while write setting" << id << res;
        return res;
    }
    return QString();
}

QVariant SettingsManager::readValue(const QString& id, const QVariant& defValue, bool *hasValue) const
{
    auto table = settingsTable();

    SelectQuery query(table->sqlSelectById(id));
    if (query.isFailed())
    {
        qWarning() << "Unable to read setting" << id << query.error();
        return defValue;
    }
    if (!query.next())
    {
        if (hasValue)
            *hasValue = false;
        return defValue;
    }
    if (hasValue)
        *hasValue = true;
    return query.record().field(table->value).value();
}

QString SettingsManager::writeString(const QString& id, const QString& value) const
{
    bool hasValue = false;
    QString oldValue = readValue(id, QVariant(), &hasValue).toString();
    if (!hasValue || oldValue != value)
        return writeValue(id, value);
    return QString();
}

QString SettingsManager::readString(const QString& id, const QString& defValue) const
{
    return readValue(id, defValue).toString();
}

QString SettingsManager::writeBool(const QString& id, bool value) const
{
    bool hasValue = false;
    bool oldValue = readValue(id, QVariant(), &hasValue).toBool();
    if (!hasValue || oldValue != value)
        return writeValue(id, value);
    return QString();
}

bool SettingsManager::readBool(const QString& id, bool defValue) const
{
    return readValue(id, defValue).toBool();
}

QString SettingsManager::writeInt(const QString& id, int value) const
{
    bool hasValue = false;
    int oldValue = readValue(id, QVariant(), &hasValue).toInt();
    if (!hasValue || oldValue != value)
        return writeValue(id, value);
    return QString();
}

int SettingsManager::readInt(const QString& id, int defValue) const
{
    return readValue(id, defValue).toInt();
}

QString SettingsManager::writeIntArray(const QString& id, const QVector<int>& values, TrackChangesFlag trackChangesFlag) const
{
    if (trackChangesFlag == IgnoreValuesOrder)
    {
        bool doSave = false;
        auto oldValues = readIntArray(id);
        for (int value: oldValues)
            if (!values.contains(value))
            {
                doSave = true;
                break;
            }
        if (!doSave)
            for (int value: values)
                if (!oldValues.contains(value))
                {
                    doSave = true;
                    break;
                }
        if (!doSave) return QString();
    }

    QString valueStr;
    QStringList strs;
    for (int value: values)
        strs << QString::number(value);
    valueStr = strs.join(';');

    if (trackChangesFlag == RespectValuesOrder)
    {
        auto oldValueStr = readValue(id).toString();
        if (oldValueStr == valueStr) return QString();
    }

    return writeValue(id, valueStr);
}

QVector<int> SettingsManager::readIntArray(const QString& id) const
{
    QVector<int> result;
    QString valuesStr = readValue(id).toString();
    if (!valuesStr.isEmpty())
    {
        bool ok;
        int value;
        for (const QString& valueStr: valuesStr.split(';'))
        {
            value = valueStr.toInt(&ok);
            if (ok) result << value;
        }
    }
    return result;
}
