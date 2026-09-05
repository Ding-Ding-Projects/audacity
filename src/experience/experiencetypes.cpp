/*
 * Audacity: A Digital Audio Editor
 */
#include "experiencetypes.h"

#include <QDate>
#include <QUrl>
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

namespace {
bool isValidBoundDate(const QString& iso)
{
    return iso.isEmpty() || QDate::fromString(iso, Qt::ISODate).isValid();
}
}

bool ScheduleEntry::isValid() const
{
    if (hour < 0 || hour > 23) {
        return false;
    }
    if (minute < 0 || minute > 59) {
        return false;
    }
    if (hasEndTime() && (endHour > 23 || endMinute > 59)) {
        return false;
    }
    if ((weekdayMask & 0b1111111) == 0) {
        return false;
    }
    if (key.isEmpty()) {
        return false;
    }
    if (!isValidBoundDate(startDate) || !isValidBoundDate(endDate)) {
        return false;
    }

    if (source == ScheduleSource::HttpsApi) {
        const QUrl url(apiUrl);
        if (!url.isValid() || url.scheme() != QLatin1String("https")) {
            return false;
        }
    } else if (source == ScheduleSource::HomeAssistant) {
        const QUrl url(haBaseUrl);
        if (!url.isValid() || haEntityId.isEmpty()) {
            return false;
        }
        if (url.scheme() != QLatin1String("https")
            && !(url.scheme() == QLatin1String("http") && (url.host() == QLatin1String("127.0.0.1") || url.host() == QLatin1String(
                                                               "localhost")))) {
            return false;
        }
    } else if (source != ScheduleSource::Local) {
        return false;
    } else if (value.isEmpty()) {
        // A local row with nothing to apply is not a usable row. A remote
        // row is allowed to start with an empty starting value; it fills in
        // once the source answers.
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
    map["endHour"] = endHour;
    map["endMinute"] = endMinute;
    map["startDate"] = startDate;
    map["endDate"] = endDate;
    map["weekdayMask"] = weekdayMask;
    map["key"] = key;
    map["value"] = value;
    map["source"] = source;
    map["apiUrl"] = apiUrl;
    map["haBaseUrl"] = haBaseUrl;
    map["haEntityId"] = haEntityId;
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
    entry.endHour = map.value("endHour", -1).toInt();
    entry.endMinute = map.value("endMinute", -1).toInt();
    entry.startDate = map.value("startDate").toString();
    entry.endDate = map.value("endDate").toString();
    entry.weekdayMask = map.value("weekdayMask", 0b1111111).toInt();
    entry.key = map.value("key").toString();
    entry.value = map.value("value").toString();
    entry.source = map.value("source", ScheduleSource::Local).toString();
    entry.apiUrl = map.value("apiUrl").toString();
    entry.haBaseUrl = map.value("haBaseUrl").toString();
    entry.haEntityId = map.value("haEntityId").toString();
    return entry;
}
}
