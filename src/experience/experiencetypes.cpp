/*
 * Audacity: A Digital Audio Editor
 */
#include "experiencetypes.h"

#include <QUuid>

namespace au::experience {
QVariantMap Notification::toMap() const
{
    QVariantMap map;
    map["id"] = id;
    map["type"] = static_cast<int>(type);
    map["title"] = title;
    map["body"] = body;
    map["actionText"] = actionText;
    map["actionCode"] = actionCode;
    map["timestamp"] = timestamp;
    map["dismissed"] = dismissed;
    return map;
}

bool ScheduleEntry::isValid() const
{
    if (hour < 0 || hour > 23) {
        return false;
    }
    if (minute < 0 || minute > 59) {
        return false;
    }
    if ((weekdayMask & 0b1111111) == 0) {
        return false;
    }
    if (key.isEmpty() || value.isEmpty()) {
        return false;
    }
    return key == ScheduleKeys::LanguageMode
           || key == ScheduleKeys::Theme
           || key == ScheduleKeys::Density
           || key == ScheduleKeys::SeedColor
           || key == ScheduleKeys::FontFamily
           || key == ScheduleKeys::FontSize
           || key == ScheduleKeys::ReducedMotion;
}

QVariantMap ScheduleEntry::toMap() const
{
    QVariantMap map;
    map["id"] = id;
    map["enabled"] = enabled;
    map["hour"] = hour;
    map["minute"] = minute;
    map["weekdayMask"] = weekdayMask;
    map["key"] = key;
    map["value"] = value;
    return map;
}

ScheduleEntry ScheduleEntry::fromMap(const QVariantMap& map)
{
    ScheduleEntry entry;
    entry.id = map.value("id").toString();
    if (entry.id.isEmpty()) {
        entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    entry.enabled = map.value("enabled", true).toBool();
    entry.hour = map.value("hour", 9).toInt();
    entry.minute = map.value("minute", 0).toInt();
    entry.weekdayMask = map.value("weekdayMask", 0b1111111).toInt();
    entry.key = map.value("key").toString();
    entry.value = map.value("value").toString();
    return entry;
}
}
