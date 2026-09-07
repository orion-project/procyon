#ifndef GRID_VIEW_COLUMNS_H
#define GRID_VIEW_COLUMNS_H

#include <QString>
#include <QHeaderView>
#include <QJsonObject>

class Memo;

namespace {

enum class ColumnKind { NONE, ID, PROP };

struct ColumnDef
{
    ColumnKind kind = ColumnKind::NONE;
    std::function<QString()> header;
    std::function<QVariant(Memo*)> value;
    QHeaderView::ResizeMode resizeMode = QHeaderView::ResizeToContents;
};

struct ValueFormat
{
    template <typename T> struct Look
    {
        T value;
        bool fullRow = false;
        QJsonObject toJson() const
        {
            QJsonObject json;
            if constexpr (std::is_same_v<T, QColor>)
                json[QLatin1String("value")] = value.name();
            else
                json[QLatin1String("value")] = value;
            if (fullRow)
                json[QLatin1String("fullRow")] = true;
            return json;
        }
    };
    std::optional<Look<QColor>> backColor;
    std::optional<Look<QColor>> textColor;
    std::optional<Look<bool>> fontB;
    std::optional<Look<bool>> fontI;
    std::optional<Look<bool>> fontU;
    std::optional<Look<bool>> fontS;
    
    template <typename T>
    static std::optional<Look<T>> fromJson(const QJsonObject& obj, QLatin1StringView key)
    {
        if (!obj.contains(key)) return {};
        auto o = obj[key].toObject();
        if (o.isEmpty()) return {};
        if constexpr (std::is_same_v<T, QColor>)
        {
            return Look<T> {
                .value = QColor(o[QLatin1String("value")].toString()),
                .fullRow = o[QLatin1String("fullRow")].toBool()
            };
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            return Look<T> {
                .value = o[QLatin1String("value")].toBool(),
                .fullRow = o[QLatin1String("fullRow")].toBool()
            };
        }
        qWarning() << "Unsupported property format value type";
        return {};
    }

    bool isEmpty() const
    {
        return !backColor && !textColor && !fontB && !fontI && !fontU && !fontS;
    }
};

using PropFormat = QHash<QString, ValueFormat>;
using PropFormats = QHash<QString, PropFormat>;

}

#endif // GRID_VIEW_COLUMNS_H
