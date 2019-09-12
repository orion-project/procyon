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

void SettingsManager::writeValue(const QString& id, const QVariant& value) const
{
    auto table = settingsTable();

    SelectQuery query(table->sqlCheckId(id));
    if (query.isFailed())
    {
        qWarning() << "Unable to write setting" << id << query.error();
        return;
    }
    QString sql = query.next() ? table->sqlUpdate : table->sqlInsert;
    QString res = ActionQuery(sql)
            .param(table->id, id)
            .param(table->value, value)
            .exec();
    if (!res.isEmpty())
        qWarning() << "Error while write setting" << id << res;
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

void SettingsManager::writeBool(const QString& id, bool value) const
{
    bool hasValue;
    bool oldValue = readValue(id, QVariant(), &hasValue).toBool();
    if (!hasValue || oldValue != value)
        writeValue(id, value);
}

bool SettingsManager::readBool(const QString& id, bool defValue) const
{
    return readValue(id, defValue).toBool();
}

void SettingsManager::writeInt(const QString& id, int value) const
{
    bool hasValue;
    int oldValue = readValue(id, QVariant(), &hasValue).toInt();
    if (!hasValue || oldValue != value)
        writeValue(id, value);
}

int SettingsManager::readInt(const QString& id, int defValue) const
{
    return readValue(id, defValue).toInt();
}

void SettingsManager::writeIntArray(const QString& id, const QVector<int>& values, TrackChangesFlag trackChangesFlag) const
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
        if (!doSave) return;
    }

    QString valueStr;
    QStringList strs;
    for (int value: values)
        strs << QString::number(value);
    valueStr = strs.join(';');

    if (trackChangesFlag == RespectValuesOrder)
    {
        auto oldValueStr = readValue(id).toString();
        if (oldValueStr == valueStr) return;
    }

    writeValue(id, valueStr);
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
