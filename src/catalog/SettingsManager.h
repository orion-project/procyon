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

    void writeValue(const QString& id, const QVariant& value) const;
    QVariant readValue(const QString& id, const QVariant& defValue = QVariant(), bool *hasValue = nullptr) const;

    void writeString(const QString& id, const QString& value) const;
    QString readString(const QString& id, const QString& defValue = QString()) const;

    void writeBool(const QString& id, bool value) const;
    bool readBool(const QString& id, bool defValue) const;

    void writeInt(const QString& id, int value) const;
    int readInt(const QString& id, int defValue) const;

    void writeIntArray(const QString& id, const QVector<int>& values,
                       TrackChangesFlag trackChangesFlag = IgnoreValuesOrder) const;
    QVector<int> readIntArray(const QString& id) const;
};

#endif // SETTINGS_MANAGER_H
