/*
 * Audacity: A Digital Audio Editor
 */
#include "schedulelistmodel.h"

#include <QDateTime>
#include <QLocale>
#include <QStringList>
#include <QUuid>

#include "framework/global/translation.h"

#include "internal/settingsscheduler.h"

namespace au::experience {
namespace {
QVariantMap choice(const QString& value, const QString& title)
{
    QVariantMap map;
    map["value"] = value;
    map["title"] = title;
    return map;
}
}

ScheduleListModel::ScheduleListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

void ScheduleListModel::load()
{
    reload();
    configuration()->scheduleChanged().onNotify(this, [this]() {
        reload();
    });
}

void ScheduleListModel::reload()
{
    beginResetModel();
    m_entries = configuration()->schedule();
    endResetModel();
    emit countChanged();
}

int ScheduleListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(m_entries.size());
}

QHash<int, QByteArray> ScheduleListModel::roleNames() const
{
    return {
        { IdRole, "entryId" },
        { EnabledRole, "entryEnabled" },
        { HourRole, "hour" },
        { MinuteRole, "minute" },
        { WeekdayMaskRole, "weekdayMask" },
        { KeyRole, "settingKey" },
        { ValueRole, "settingValue" },
        { TimeTextRole, "timeText" },
        { DaysTextRole, "daysText" },
        { SettingTextRole, "settingText" },
        { NextFireTextRole, "nextFireText" }
    };
}

QString ScheduleListModel::settingText(const ScheduleEntry& entry) const
{
    for (const QVariant& item : availableSettings()) {
        const QVariantMap setting = item.toMap();
        if (setting.value("key").toString() != entry.key) {
            continue;
        }

        const QString title = setting.value("title").toString();
        for (const QVariant& choiceItem : setting.value("choices").toList()) {
            const QVariantMap map = choiceItem.toMap();
            if (map.value("value").toString() == entry.value) {
                return title + QStringLiteral(": ") + map.value("title").toString();
            }
        }
        return title + QStringLiteral(": ") + entry.value;
    }
    return entry.key + QStringLiteral(": ") + entry.value;
}

QVariant ScheduleListModel::data(const QModelIndex& index, int role) const
{
    if (index.row() < 0 || index.row() >= rowCount()) {
        return QVariant();
    }

    const ScheduleEntry& entry = m_entries.at(static_cast<size_t>(index.row()));

    switch (role) {
    case IdRole:
        return entry.id;
    case EnabledRole:
        return entry.enabled;
    case HourRole:
        return entry.hour;
    case MinuteRole:
        return entry.minute;
    case WeekdayMaskRole:
        return entry.weekdayMask;
    case KeyRole:
        return entry.key;
    case ValueRole:
        return entry.value;
    case TimeTextRole:
        return QStringLiteral("%1:%2").arg(entry.hour, 2, 10, QLatin1Char('0')).arg(entry.minute, 2, 10, QLatin1Char('0'));
    case DaysTextRole: {
        if ((entry.weekdayMask & 0b1111111) == 0b1111111) {
            return muse::qtrc("experience", "Every day");
        }
        QStringList days;
        const QLocale locale;
        for (int bit = 0; bit < 7; ++bit) {
            if (entry.weekdayMask & (1 << bit)) {
                days.append(locale.dayName(bit + 1, QLocale::ShortFormat));
            }
        }
        return days.join(QStringLiteral(", "));
    }
    case SettingTextRole:
        return settingText(entry);
    case NextFireTextRole: {
        const QDateTime next = SettingsScheduler::nextFire(entry, QDateTime::currentDateTime());
        if (!next.isValid()) {
            return muse::qtrc("experience", "Not scheduled");
        }
        return muse::qtrc("experience", "Next: %1").arg(QLocale().toString(next, QLocale::ShortFormat));
    }
    default:
        break;
    }

    return QVariant();
}

QString ScheduleListModel::save(const QVariantMap& row)
{
    ScheduleEntry entry = ScheduleEntry::fromMap(row);
    if (!entry.isValid()) {
        return QString();
    }

    std::vector<ScheduleEntry> entries = m_entries;
    bool replaced = false;
    for (ScheduleEntry& existing : entries) {
        if (existing.id == entry.id) {
            existing = entry;
            replaced = true;
            break;
        }
    }

    if (!replaced) {
        entries.push_back(entry);
    }

    configuration()->setSchedule(entries);
    reload();
    return entry.id;
}

void ScheduleListModel::remove(const QString& id)
{
    std::vector<ScheduleEntry> entries;
    for (const ScheduleEntry& entry : m_entries) {
        if (entry.id != id) {
            entries.push_back(entry);
        }
    }

    configuration()->setSchedule(entries);
    reload();
}

void ScheduleListModel::setEnabled(const QString& id, bool enabled)
{
    std::vector<ScheduleEntry> entries = m_entries;
    for (ScheduleEntry& entry : entries) {
        if (entry.id == id) {
            entry.enabled = enabled;
        }
    }

    configuration()->setSchedule(entries);
    reload();
}

QVariantMap ScheduleListModel::row(const QString& id) const
{
    for (const ScheduleEntry& entry : m_entries) {
        if (entry.id == id) {
            return entry.toMap();
        }
    }

    ScheduleEntry fresh;
    fresh.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    fresh.key = ScheduleKeys::Theme;
    fresh.value = QStringLiteral("dark");
    return fresh.toMap();
}

QVariantList ScheduleListModel::availableSettings() const
{
    QVariantList settings;

    QVariantMap language;
    language["key"] = ScheduleKeys::LanguageMode;
    language["title"] = muse::qtrc("experience", "Language mode");
    language["choices"] = QVariantList {
        choice(QStringLiteral("english"), muse::qtrc("experience", "English")),
        choice(QStringLiteral("cantonese"), muse::qtrc("experience", "Cantonese (Hong Kong)")),
        choice(QStringLiteral("bilingual"), muse::qtrc("experience", "Bilingual"))
    };
    settings.append(language);

    QVariantMap theme;
    theme["key"] = ScheduleKeys::Theme;
    theme["title"] = muse::qtrc("experience", "Theme");
    theme["choices"] = QVariantList {
        choice(QStringLiteral("light"), muse::qtrc("experience", "Light")),
        choice(QStringLiteral("dark"), muse::qtrc("experience", "Dark")),
        choice(QStringLiteral("system"), muse::qtrc("experience", "Follow the system"))
    };
    settings.append(theme);

    QVariantMap density;
    density["key"] = ScheduleKeys::Density;
    density["title"] = muse::qtrc("experience", "Density");
    density["choices"] = QVariantList {
        choice(QStringLiteral("0"), muse::qtrc("experience", "Default")),
        choice(QStringLiteral("-1"), muse::qtrc("experience", "Comfortable")),
        choice(QStringLiteral("-2"), muse::qtrc("experience", "Compact")),
        choice(QStringLiteral("-3"), muse::qtrc("experience", "Very compact"))
    };
    settings.append(density);

    QVariantMap seed;
    seed["key"] = ScheduleKeys::SeedColor;
    seed["title"] = muse::qtrc("experience", "Seed colour");
    seed["freeText"] = true;
    seed["placeholder"] = QStringLiteral("#926BFF");
    seed["choices"] = QVariantList {
        choice(QStringLiteral("#926BFF"), muse::qtrc("experience", "Violet")),
        choice(QStringLiteral("#00639B"), muse::qtrc("experience", "Blue")),
        choice(QStringLiteral("#3F6837"), muse::qtrc("experience", "Green")),
        choice(QStringLiteral("#8F4C38"), muse::qtrc("experience", "Terracotta"))
    };
    settings.append(seed);

    QVariantMap fontFamily;
    fontFamily["key"] = ScheduleKeys::FontFamily;
    fontFamily["title"] = muse::qtrc("experience", "Font family");
    fontFamily["freeText"] = true;
    fontFamily["placeholder"] = QStringLiteral("Roboto Flex");
    fontFamily["choices"] = QVariantList {
        choice(QStringLiteral("Roboto Flex"), QStringLiteral("Roboto Flex")),
        choice(QStringLiteral("Noto Sans HK"), QStringLiteral("Noto Sans HK"))
    };
    settings.append(fontFamily);

    QVariantMap fontSize;
    fontSize["key"] = ScheduleKeys::FontSize;
    fontSize["title"] = muse::qtrc("experience", "Font size");
    fontSize["choices"] = QVariantList {
        choice(QStringLiteral("12"), QStringLiteral("12")),
        choice(QStringLiteral("14"), QStringLiteral("14")),
        choice(QStringLiteral("16"), QStringLiteral("16")),
        choice(QStringLiteral("18"), QStringLiteral("18"))
    };
    settings.append(fontSize);

    QVariantMap reducedMotion;
    reducedMotion["key"] = ScheduleKeys::ReducedMotion;
    reducedMotion["title"] = muse::qtrc("experience", "Reduced motion");
    reducedMotion["choices"] = QVariantList {
        choice(QStringLiteral("on"), muse::qtrc("experience", "On")),
        choice(QStringLiteral("off"), muse::qtrc("experience", "Off"))
    };
    settings.append(reducedMotion);

    return settings;
}
}
