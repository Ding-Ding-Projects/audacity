/*
 * Audacity: A Digital Audio Editor
 */
#include "settingsscheduler.h"

#include <QRegularExpression>
#include <QTimer>

#include "framework/global/settings.h"
#include "log.h"

using namespace muse;

namespace au::experience {
namespace {
//! The scheduler looks at the clock once a minute. A row fires at most once
//! per occurrence, because a tick only fires the rows whose minute falls
//! inside the interval that just passed.
constexpr int TICK_INTERVAL_MS = 60 * 1000;

static const Settings::Key UI_CURRENT_THEME_CODE("ui", "ui/application/currentThemeCode");
static const Settings::Key UI_FOLLOW_SYSTEM_THEME("ui", "ui/application/followSystemTheme");
static const Settings::Key UI_FONT_FAMILY("ui", "ui/theme/fontFamily");
static const Settings::Key UI_FONT_SIZE("ui", "ui/theme/fontSize");
static const Settings::Key M3_SEED_COLOR("ui", "ui/m3/seedColor");
static const Settings::Key M3_REDUCED_MOTION("ui", "ui/m3/reducedMotion");
static const Settings::Key M3_DENSITY("ui", "ui/m3/density");

bool firesOnDay(const ScheduleEntry& entry, const QDate& date)
{
    //! Qt numbers Monday as 1 and Sunday as 7. Bit 0 is Monday.
    const int bit = date.dayOfWeek() - 1;
    return (entry.weekdayMask & (1 << bit)) != 0;
}
}

SettingsScheduler::SettingsScheduler(QObject* parent)
    : QObject(parent)
{
}

void SettingsScheduler::init()
{
    m_lastTick = QDateTime::currentDateTime();

    auto* timer = new QTimer(this);
    timer->setInterval(TICK_INTERVAL_MS);
    timer->setTimerType(Qt::CoarseTimer);
    connect(timer, &QTimer::timeout, this, &SettingsScheduler::tick);
    timer->start();
}

QDateTime SettingsScheduler::nextFire(const ScheduleEntry& entry, const QDateTime& from)
{
    if (!entry.enabled || !entry.isValid() || !from.isValid()) {
        return QDateTime();
    }

    const QTime time(entry.hour, entry.minute);
    for (int offset = 0; offset <= 7; ++offset) {
        const QDate date = from.date().addDays(offset);
        if (!firesOnDay(entry, date)) {
            continue;
        }

        const QDateTime candidate(date, time);
        if (candidate >= from) {
            return candidate;
        }
    }

    return QDateTime();
}

bool SettingsScheduler::applyEntry(const ScheduleEntry& entry)
{
    if (!entry.isValid()) {
        return false;
    }

    if (entry.key == ScheduleKeys::LanguageMode) {
        if (entry.value == QLatin1String("english")) {
            configuration()->setLanguageMode(LanguageMode::English);
        } else if (entry.value == QLatin1String("cantonese")) {
            configuration()->setLanguageMode(LanguageMode::Cantonese);
        } else if (entry.value == QLatin1String("bilingual")) {
            configuration()->setLanguageMode(LanguageMode::Bilingual);
        } else {
            return false;
        }
        return true;
    }

    if (entry.key == ScheduleKeys::Theme) {
        if (entry.value == QLatin1String("system")) {
            settings()->setSharedValue(UI_FOLLOW_SYSTEM_THEME, Val(true));
            return true;
        }
        if (entry.value != QLatin1String("light") && entry.value != QLatin1String("dark")) {
            return false;
        }
        settings()->setSharedValue(UI_FOLLOW_SYSTEM_THEME, Val(false));
        settings()->setSharedValue(UI_CURRENT_THEME_CODE,
                                   Val(entry.value == QLatin1String("light") ? std::string("light") : std::string("dark")));
        return true;
    }

    if (entry.key == ScheduleKeys::Density) {
        bool ok = false;
        const int level = entry.value.toInt(&ok);
        if (!ok || level > 0 || level < -3) {
            return false;
        }
        settings()->setSharedValue(M3_DENSITY, Val(level));
        return true;
    }

    if (entry.key == ScheduleKeys::SeedColor) {
        static const QRegularExpression hexColor(QStringLiteral("^#[0-9A-Fa-f]{6}$"));
        if (!hexColor.match(entry.value).hasMatch()) {
            return false;
        }
        settings()->setSharedValue(M3_SEED_COLOR, Val(entry.value.toLower().toStdString()));
        return true;
    }

    if (entry.key == ScheduleKeys::FontFamily) {
        settings()->setSharedValue(UI_FONT_FAMILY, Val(entry.value.toStdString()));
        return true;
    }

    if (entry.key == ScheduleKeys::FontSize) {
        bool ok = false;
        const int size = entry.value.toInt(&ok);
        if (!ok || size < 8 || size > 32) {
            return false;
        }
        settings()->setSharedValue(UI_FONT_SIZE, Val(size));
        return true;
    }

    if (entry.key == ScheduleKeys::ReducedMotion) {
        const bool on = entry.value == QLatin1String("on") || entry.value == QLatin1String("true");
        settings()->setSharedValue(M3_REDUCED_MOTION, Val(on));
        return true;
    }

    return false;
}

void SettingsScheduler::tick()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime previous = m_lastTick.isValid() ? m_lastTick : now;
    m_lastTick = now;

    if (previous >= now) {
        return;
    }

    for (const ScheduleEntry& entry : configuration()->schedule()) {
        const QDateTime due = nextFire(entry, previous);
        if (due.isValid() && due > previous && due <= now) {
            if (!applyEntry(entry)) {
                LOGW() << "Scheduled setting row could not be applied: " << entry.key;
            }
        }
    }
}
}
