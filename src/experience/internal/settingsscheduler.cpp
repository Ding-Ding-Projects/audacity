/*
 * Audacity: A Digital Audio Editor
 */
#include "settingsscheduler.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>

#include "framework/global/settings.h"
#include "log.h"

using namespace muse;

namespace au::experience {
namespace {
//! The scheduler looks at the clock once a minute. A row fires at most once
//! per occurrence, because a tick only fires the rows whose minute falls
//! inside the interval that just passed.
constexpr int TICK_INTERVAL_MS = 60 * 1000;

//! A remote row's answer is never trusted blindly: the response body is
//! capped, the request times out quickly, and redirects are refused so a
//! compromised or misconfigured endpoint cannot quietly redirect the app
//! somewhere else.
constexpr qint64 MAX_RESPONSE_BYTES = 64 * 1024;
constexpr int NETWORK_TIMEOUT_MS = 5000;

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

bool withinDateBounds(const ScheduleEntry& entry, const QDate& date)
{
    if (!entry.startDate.isEmpty()) {
        const QDate start = QDate::fromString(entry.startDate, Qt::ISODate);
        if (start.isValid() && date < start) {
            return false;
        }
    }
    if (!entry.endDate.isEmpty()) {
        const QDate end = QDate::fromString(entry.endDate, Qt::ISODate);
        if (end.isValid() && date > end) {
            return false;
        }
    }
    return true;
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

    // Resolve remote rows once at start-up too, so the first tick after
    // launch already has an answer to work with rather than waiting a
    // whole minute.
    tick();
}

QDateTime SettingsScheduler::nextFire(const ScheduleEntry& entry, const QDateTime& from)
{
    if (!entry.enabled || !entry.isValid() || !from.isValid()) {
        return QDateTime();
    }

    const QTime time(entry.hour, entry.minute);
    for (int offset = 0; offset <= 7; ++offset) {
        const QDate date = from.date().addDays(offset);
        if (!firesOnDay(entry, date) || !withinDateBounds(entry, date)) {
            continue;
        }

        const QDateTime candidate(date, time);
        if (candidate >= from) {
            return candidate;
        }
    }

    return QDateTime();
}

bool SettingsScheduler::isWithinWindow(const ScheduleEntry& entry, const QDateTime& now)
{
    if (!entry.enabled || !entry.isValid() || !now.isValid()) {
        return false;
    }
    if (!firesOnDay(entry, now.date()) || !withinDateBounds(entry, now.date())) {
        return false;
    }

    const QTime time = now.time();
    const QTime start(entry.hour, entry.minute);

    if (!entry.hasEndTime()) {
        // No window: only the exact fire minute counts, which the caller
        // establishes by asking about "now" at tick time. isWithinWindow is
        // used for the window case; one-shot rows are handled by nextFire.
        return time.hour() == start.hour() && time.minute() == start.minute();
    }

    const QTime end(entry.endHour, entry.endMinute);
    if (start <= end) {
        return time >= start && time < end;
    }
    // The window crosses midnight.
    return time >= start || time < end;
}

QString SettingsScheduler::currentValueFor(const QString& key) const
{
    if (key == ScheduleKeys::LanguageMode) {
        switch (configuration()->languageMode()) {
        case LanguageMode::English: return QStringLiteral("english");
        case LanguageMode::Cantonese: return QStringLiteral("cantonese");
        case LanguageMode::Bilingual: return QStringLiteral("bilingual");
        }
    }
    if (key == ScheduleKeys::Theme) {
        if (settings()->value(UI_FOLLOW_SYSTEM_THEME).toBool()) {
            return QStringLiteral("system");
        }
        return QString::fromStdString(settings()->value(UI_CURRENT_THEME_CODE).toString());
    }
    if (key == ScheduleKeys::Density) {
        return QString::number(settings()->value(M3_DENSITY).toInt());
    }
    if (key == ScheduleKeys::SeedColor) {
        return QString::fromStdString(settings()->value(M3_SEED_COLOR).toString());
    }
    if (key == ScheduleKeys::FontFamily) {
        return QString::fromStdString(settings()->value(UI_FONT_FAMILY).toString());
    }
    if (key == ScheduleKeys::FontSize) {
        return QString::number(settings()->value(UI_FONT_SIZE).toInt());
    }
    if (key == ScheduleKeys::ReducedMotion) {
        return settings()->value(M3_REDUCED_MOTION).toBool() ? QStringLiteral("on") : QStringLiteral("off");
    }
    return QString();
}

bool SettingsScheduler::applyEntry(const ScheduleEntry& entry, const QString& value)
{
    if (entry.key == ScheduleKeys::LanguageMode) {
        if (value == QLatin1String("english")) {
            configuration()->setLanguageMode(LanguageMode::English);
        } else if (value == QLatin1String("cantonese")) {
            configuration()->setLanguageMode(LanguageMode::Cantonese);
        } else if (value == QLatin1String("bilingual")) {
            configuration()->setLanguageMode(LanguageMode::Bilingual);
        } else {
            return false;
        }
        return true;
    }

    if (entry.key == ScheduleKeys::Theme) {
        if (value == QLatin1String("system")) {
            settings()->setSharedValue(UI_FOLLOW_SYSTEM_THEME, Val(true));
            return true;
        }
        if (value != QLatin1String("light") && value != QLatin1String("dark")) {
            return false;
        }
        settings()->setSharedValue(UI_FOLLOW_SYSTEM_THEME, Val(false));
        settings()->setSharedValue(UI_CURRENT_THEME_CODE,
                                   Val(value == QLatin1String("light") ? std::string("light") : std::string("dark")));
        return true;
    }

    if (entry.key == ScheduleKeys::Density) {
        bool ok = false;
        const int level = value.toInt(&ok);
        if (!ok || level > 0 || level < -3) {
            return false;
        }
        settings()->setSharedValue(M3_DENSITY, Val(level));
        return true;
    }

    if (entry.key == ScheduleKeys::SeedColor) {
        static const QRegularExpression hexColor(QStringLiteral("^#[0-9A-Fa-f]{6}$"));
        if (!hexColor.match(value).hasMatch()) {
            return false;
        }
        settings()->setSharedValue(M3_SEED_COLOR, Val(value.toLower().toStdString()));
        return true;
    }

    if (entry.key == ScheduleKeys::FontFamily) {
        if (value.isEmpty()) {
            return false;
        }
        settings()->setSharedValue(UI_FONT_FAMILY, Val(value.toStdString()));
        return true;
    }

    if (entry.key == ScheduleKeys::FontSize) {
        bool ok = false;
        const int size = value.toInt(&ok);
        if (!ok || size < 8 || size > 32) {
            return false;
        }
        settings()->setSharedValue(UI_FONT_SIZE, Val(size));
        return true;
    }

    if (entry.key == ScheduleKeys::ReducedMotion) {
        const bool on = value == QLatin1String("on") || value == QLatin1String("true");
        settings()->setSharedValue(M3_REDUCED_MOTION, Val(on));
        return true;
    }

    return false;
}

void SettingsScheduler::refreshRemoteValue(const ScheduleEntry& entry)
{
    QNetworkRequest request;
    request.setTransferTimeout(NETWORK_TIMEOUT_MS);
    // Never follow a redirect: the endpoint the user typed is the one
    // endpoint that gets trusted with a request.
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);

    const QString entryId = entry.id;
    const QString key = entry.key;

    if (entry.source == ScheduleSource::HttpsApi) {
        request.setUrl(QUrl(entry.apiUrl));
    } else if (entry.source == ScheduleSource::HomeAssistant) {
        QUrl url(entry.haBaseUrl);
        url.setPath(url.path() + QStringLiteral("/api/states/") + entry.haEntityId);
        request.setUrl(url);
        const QString token = configuration()->homeAssistantToken();
        if (!token.isEmpty()) {
            request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
        }
    } else {
        return;
    }

    QNetworkReply* reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, entryId, key, source = entry.source, fallback = entry.value]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            LOGW() << "Scheduled setting source unreachable for row " << entryId << ": " << reply->errorString();
            return;
        }

        const QByteArray body = reply->read(MAX_RESPONSE_BYTES + 1);
        if (body.size() > MAX_RESPONSE_BYTES) {
            LOGW() << "Scheduled setting source answered with more than the allowed size, ignoring it";
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(body);
        if (!doc.isObject()) {
            return;
        }
        const QJsonObject obj = doc.object();

        QString resolved;
        if (source == ScheduleSource::HttpsApi) {
            // Only the exact field named by the row's own key is trusted:
            // every other field in the response is ignored.
            if (!obj.contains(key)) {
                return;
            }
            resolved = obj.value(key).toVariant().toString();
        } else {
            // Home Assistant answers with the boolean entity's own state.
            const QString state = obj.value(QStringLiteral("state")).toString();
            if (state != QLatin1String("on") && state != QLatin1String("off")) {
                return;
            }
            resolved = state == QLatin1String("on") ? fallback : QString();
        }

        if (!resolved.isEmpty()) {
            m_remoteValues[entryId] = resolved;
        } else {
            m_remoteValues.remove(entryId);
        }
    });
}

void SettingsScheduler::tick()
{
    const QDateTime now = QDateTime::currentDateTime();
    const QDateTime previous = m_lastTick.isValid() ? m_lastTick : now;
    m_lastTick = now;

    QSet<QString> stillActive;

    for (const ScheduleEntry& entry : configuration()->schedule()) {
        if (!entry.enabled || !entry.isValid()) {
            continue;
        }

        if (entry.source != ScheduleSource::Local) {
            refreshRemoteValue(entry);
        }

        const QString resolvedValue = m_remoteValues.value(entry.id, entry.value);

        if (entry.hasEndTime()) {
            const bool active = isWithinWindow(entry, now);
            if (active) {
                stillActive.insert(entry.id);
                if (!m_activeRows.contains(entry.id)) {
                    m_baseValues[entry.id] = currentValueFor(entry.key);
                    if (!applyEntry(entry, resolvedValue)) {
                        LOGW() << "Scheduled setting row could not be applied: " << entry.key;
                    }
                }
            } else if (m_activeRows.contains(entry.id)) {
                const QString base = m_baseValues.take(entry.id);
                if (!base.isEmpty()) {
                    applyEntry(entry, base);
                }
            }
            continue;
        }

        // One-shot rows keep their original semantics: fire exactly once,
        // the moment the clock passes their minute.
        const QDateTime due = nextFire(entry, previous);
        if (due.isValid() && due > previous && due <= now) {
            if (!applyEntry(entry, resolvedValue)) {
                LOGW() << "Scheduled setting row could not be applied: " << entry.key;
            }
        }
    }

    m_activeRows = stillActive;
}
}
