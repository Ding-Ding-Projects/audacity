/*
* Audacity: A Digital Audio Editor
*/
#include "squirrelupdateconfiguration.h"

#include "framework/global/settings.h"

using namespace muse;

namespace au::squirrelupdate {
namespace {
const std::string moduleName("squirrelupdate");

const Settings::Key ENABLED(moduleName, "squirrelupdate/enabled");
const Settings::Key FEED_URL(moduleName, "squirrelupdate/feedUrl");
const Settings::Key CHECK_INTERVAL_HOURS(moduleName, "squirrelupdate/checkIntervalHours");

const std::string DEFAULT_FEED_URL(
    "https://github.com/Ding-Ding-Projects/audacity/releases/latest/download/RELEASES");

constexpr int MIN_INTERVAL_HOURS = 1;
constexpr int MAX_INTERVAL_HOURS = 24 * 30;
}

void SquirrelUpdateConfiguration::init()
{
    settings()->setDefaultValue(ENABLED, Val(true));
    settings()->setDefaultValue(FEED_URL, Val(DEFAULT_FEED_URL));
    settings()->setDefaultValue(CHECK_INTERVAL_HOURS, Val(24));

    for (const Settings::Key& key : { ENABLED, FEED_URL, CHECK_INTERVAL_HOURS }) {
        settings()->valueChanged(key).onReceive(this, [this](const Val&) {
            m_changed.notify();
        });
    }
}

bool SquirrelUpdateConfiguration::isEnabled() const
{
    return settings()->value(ENABLED).toBool();
}

void SquirrelUpdateConfiguration::setEnabled(bool value)
{
    settings()->setSharedValue(ENABLED, Val(value));
}

QString SquirrelUpdateConfiguration::feedUrl() const
{
    const QString stored = QString::fromStdString(settings()->value(FEED_URL).toString()).trimmed();
    return stored.isEmpty() ? defaultFeedUrl() : stored;
}

void SquirrelUpdateConfiguration::setFeedUrl(const QString& url)
{
    settings()->setSharedValue(FEED_URL, Val(url.trimmed().toStdString()));
}

QString SquirrelUpdateConfiguration::defaultFeedUrl() const
{
    return QString::fromStdString(DEFAULT_FEED_URL);
}

int SquirrelUpdateConfiguration::checkIntervalHours() const
{
    const int stored = settings()->value(CHECK_INTERVAL_HOURS).toInt();
    if (stored < MIN_INTERVAL_HOURS) {
        return MIN_INTERVAL_HOURS;
    }
    if (stored > MAX_INTERVAL_HOURS) {
        return MAX_INTERVAL_HOURS;
    }
    return stored;
}

void SquirrelUpdateConfiguration::setCheckIntervalHours(int hours)
{
    settings()->setSharedValue(CHECK_INTERVAL_HOURS, Val(hours));
}

async::Notification SquirrelUpdateConfiguration::changed() const
{
    return m_changed;
}
}
