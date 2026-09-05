/*
 * Audacity: A Digital Audio Editor
 */
#include <gtest/gtest.h>

#include <QDateTime>

#include "internal/settingsscheduler.h"

using namespace au::experience;

namespace {
ScheduleEntry makeEntry(int hour, int minute, int weekdayMask)
{
    ScheduleEntry entry;
    entry.id = QStringLiteral("row");
    entry.enabled = true;
    entry.hour = hour;
    entry.minute = minute;
    entry.weekdayMask = weekdayMask;
    entry.key = ScheduleKeys::Theme;
    entry.value = QStringLiteral("dark");
    return entry;
}

// Monday 6 January 2025, 08:00.
const QDateTime MONDAY_MORNING(QDate(2025, 1, 6), QTime(8, 0));
}

TEST(SettingsSchedulerTests, FiresLaterTheSameDay)
{
    const ScheduleEntry entry = makeEntry(19, 30, 0b1111111);
    const QDateTime next = SettingsScheduler::nextFire(entry, MONDAY_MORNING);

    EXPECT_EQ(next, QDateTime(QDate(2025, 1, 6), QTime(19, 30)));
}

TEST(SettingsSchedulerTests, RollsOverToTheNextDayWhenTheTimeHasPassed)
{
    const ScheduleEntry entry = makeEntry(7, 0, 0b1111111);
    const QDateTime next = SettingsScheduler::nextFire(entry, MONDAY_MORNING);

    EXPECT_EQ(next, QDateTime(QDate(2025, 1, 7), QTime(7, 0)));
}

TEST(SettingsSchedulerTests, SkipsDaysThatAreNotInTheMask)
{
    // Saturday and Sunday only.
    const ScheduleEntry entry = makeEntry(9, 0, (1 << 5) | (1 << 6));
    const QDateTime next = SettingsScheduler::nextFire(entry, MONDAY_MORNING);

    EXPECT_EQ(next, QDateTime(QDate(2025, 1, 11), QTime(9, 0)));
}

TEST(SettingsSchedulerTests, FiresExactlyAtTheGivenMinute)
{
    const ScheduleEntry entry = makeEntry(8, 0, 0b1111111);
    const QDateTime next = SettingsScheduler::nextFire(entry, MONDAY_MORNING);

    EXPECT_EQ(next, MONDAY_MORNING);
}

TEST(SettingsSchedulerTests, ADisabledRowNeverFires)
{
    ScheduleEntry entry = makeEntry(19, 30, 0b1111111);
    entry.enabled = false;

    EXPECT_FALSE(SettingsScheduler::nextFire(entry, MONDAY_MORNING).isValid());
}

TEST(SettingsSchedulerTests, ARowWithNoDaysNeverFires)
{
    const ScheduleEntry entry = makeEntry(19, 30, 0);

    EXPECT_FALSE(entry.isValid());
    EXPECT_FALSE(SettingsScheduler::nextFire(entry, MONDAY_MORNING).isValid());
}

TEST(SettingsSchedulerTests, ARowNamingAnUnknownSettingIsNotValid)
{
    ScheduleEntry entry = makeEntry(19, 30, 0b1111111);
    entry.key = QStringLiteral("somethingElse");

    EXPECT_FALSE(entry.isValid());
}

TEST(SettingsSchedulerTests, OutOfRangeTimesAreNotValid)
{
    EXPECT_FALSE(makeEntry(24, 0, 0b1111111).isValid());
    EXPECT_FALSE(makeEntry(9, 60, 0b1111111).isValid());
}
