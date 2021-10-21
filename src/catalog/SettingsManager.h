#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <QString>
#include <QVector>
#include <QVariant>

class SettingsManager
{
public:
    enum TrackChangesFlag { IgnoreValuesOrder, RespectValuesOrder };

    QString prepare();

    QMap<QString, QVariant> readSettings(const QString& idPattern) const;

    QString remove(const QString& id);

    QString writeValue(const QString& id, const QVariant& value) const;
    QVariant readValue(const QString& id, const QVariant& defValue = QVariant(), bool *hasValue = nullptr) const;

    QString writeString(const QString& id, const QString& value) const;
    QString readString(const QString& id, const QString& defValue = QString()) const;

    QString writeBool(const QString& id, bool value) const;
    bool readBool(const QString& id, bool defValue) const;

    QString writeInt(const QString& id, int value) const;
    int readInt(const QString& id, int defValue) const;

    QString writeIntArray(const QString& id, const QVector<int>& values,
                       TrackChangesFlag trackChangesFlag = IgnoreValuesOrder) const;
    QVector<int> readIntArray(const QString& id) const;
};

#endif // SETTINGS_MANAGER_H
