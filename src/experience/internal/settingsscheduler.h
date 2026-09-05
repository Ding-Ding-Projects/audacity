/*
 * Audacity: A Digital Audio Editor
 */
#pragma once

#include <QDateTime>
#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QSet>

#include "framework/global/async/asyncable.h"
#include "framework/global/modularity/ioc.h"

#include "iexperienceconfiguration.h"

namespace au::experience {
//! Applies the rows of the scheduled settings table at their time of day.
//!
//! A row with no end time fires once, the way it always did. A row with an
//! end time holds a window open: the setting is applied as soon as the
//! window starts and is restored to whatever it held immediately before,
//! the moment the window closes. A row reading from an HTTPS API or a Home
//! Assistant entity resolves its value in the background at most once a
//! tick; the row keeps whatever it last resolved, or its own starting
//! value, until the next answer arrives, so a slow or unreachable source
//! never blocks the rest of the schedule.
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

    //! True when "now" falls inside the row's active window: the right days,
    //! the right time of day (or exactly the fire minute when there is no
    //! end time), and inside any start/end date bound. Pure.
    static bool isWithinWindow(const ScheduleEntry& entry, const QDateTime& now);

    //! Reads the current value of the setting a row names, in the same
    //! textual form applyEntry expects for that key.
    QString currentValueFor(const QString& key) const;

    //! Writes the setting the row names. Returns false when the row names a
    //! setting the scheduler does not know, or the value cannot be applied.
    bool applyEntry(const ScheduleEntry& entry, const QString& value);

private:
    void tick();
    void refreshRemoteValue(const ScheduleEntry& entry);

    QDateTime m_lastTick;
    QNetworkAccessManager m_network;
    //! Row id -> the value most recently resolved from a remote source.
    QHash<QString, QString> m_remoteValues;
    //! Row id -> the setting's value immediately before the row's window
    //! opened, so it can be restored when the window closes.
    QHash<QString, QString> m_baseValues;
    //! Row ids whose window was open on the previous tick.
    QSet<QString> m_activeRows;
};
}
