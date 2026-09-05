/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QDateTime>
#include <QHash>
#include <QObject>

#include "framework/global/async/asyncable.h"
#include "framework/global/modularity/ioc.h"

#include "iexperienceconfiguration.h"

namespace au::experience {
//! Applies the rows of the scheduled settings table at their time of day.
class SettingsScheduler : public QObject, public muse::async::Asyncable
{
    Q_OBJECT

    muse::GlobalInject<IExperienceConfiguration> configuration;

public:
    explicit SettingsScheduler(QObject* parent = nullptr);

    void init();

    //! The next moment at or after "from" when the row fires, or an invalid
    //! QDateTime when the row is disabled or invalid. Pure, so the tests can
    //! call it directly.
    static QDateTime nextFire(const ScheduleEntry& entry, const QDateTime& from);

    //! Writes the setting the row names. Returns false when the row names a
    //! setting the scheduler does not know.
    bool applyEntry(const ScheduleEntry& entry);

private:
    void tick();

    QDateTime m_lastTick;
};
}
