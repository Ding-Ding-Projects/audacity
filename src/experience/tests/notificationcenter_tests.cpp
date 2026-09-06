/*
 * Audacity: A Digital Audio Editor
 */
#include <gtest/gtest.h>

#include "internal/notificationcenter.h"

using namespace au::experience;

TEST(NotificationCenterTests, PushReturnsAnIncreasingId)
{
    NotificationCenter center;
    const int first = center.push(NotificationType::Info, QStringLiteral("Title"), QStringLiteral("Body"));
    const int second = center.push(NotificationType::Info, QStringLiteral("Title"), QStringLiteral("Body"));

    EXPECT_GT(second, first);
}

TEST(NotificationCenterTests, ActiveListsOnlyUndismissedNotifications)
{
    NotificationCenter center;
    const int id = center.push(NotificationType::Success, QStringLiteral("Done"), QStringLiteral("It worked"));

    ASSERT_EQ(center.active().size(), 1u);
    EXPECT_EQ(center.active().front().id, id);

    center.dismiss(id);
    EXPECT_TRUE(center.active().empty());
}

TEST(NotificationCenterTests, DismissedNotificationsStillAppearInHistory)
{
    NotificationCenter center;
    const int id = center.push(NotificationType::Warning, QStringLiteral("Careful"), QStringLiteral("Watch out"));
    center.dismiss(id);

    const std::vector<Notification> history = center.history();
    ASSERT_EQ(history.size(), 1u);
    EXPECT_TRUE(history.front().dismissed);
}

TEST(NotificationCenterTests, HistoryIsNewestFirst)
{
    NotificationCenter center;
    const int first = center.push(NotificationType::Info, QStringLiteral("One"), QStringLiteral("First"));
    const int second = center.push(NotificationType::Info, QStringLiteral("Two"), QStringLiteral("Second"));

    const std::vector<Notification> history = center.history();
    ASSERT_EQ(history.size(), 2u);
    EXPECT_EQ(history.front().id, second);
    EXPECT_EQ(history.back().id, first);
}

TEST(NotificationCenterTests, DismissAllClearsEveryActiveNotification)
{
    NotificationCenter center;
    center.push(NotificationType::Info, QStringLiteral("A"), QStringLiteral("body"));
    center.push(NotificationType::Error, QStringLiteral("B"), QStringLiteral("body"));

    center.dismissAll();

    EXPECT_TRUE(center.active().empty());
    EXPECT_EQ(center.history().size(), 2u);
}

TEST(NotificationCenterTests, ClearHistoryRemovesEverythingIncludingDismissed)
{
    NotificationCenter center;
    const int id = center.push(NotificationType::Info, QStringLiteral("A"), QStringLiteral("body"));
    center.dismiss(id);

    center.clearHistory();

    EXPECT_TRUE(center.history().empty());
    EXPECT_TRUE(center.active().empty());
}

TEST(NotificationCenterTests, DismissingAnUnknownIdDoesNothing)
{
    NotificationCenter center;
    center.push(NotificationType::Info, QStringLiteral("A"), QStringLiteral("body"));

    center.dismiss(9999);

    EXPECT_EQ(center.active().size(), 1u);
}

TEST(NotificationCenterTests, HistoryLimitDropsTheOldestEntries)
{
    NotificationCenter center;
    for (size_t i = 0; i < NotificationCenter::HISTORY_LIMIT + 10; ++i) {
        center.push(NotificationType::Info, QStringLiteral("Title"), QStringLiteral("Body %1").arg(i));
    }

    EXPECT_EQ(center.history().size(), NotificationCenter::HISTORY_LIMIT);
    //! The newest entry survives the trim; the oldest ones do not.
    EXPECT_EQ(center.history().front().body, QStringLiteral("Body %1").arg(NotificationCenter::HISTORY_LIMIT + 9));
}

TEST(NotificationCenterTests, RequestActionSendsTheActionCodeOnTheChannel)
{
    NotificationCenter center;
    QString received;
    center.actionRequested().onReceive(nullptr, [&received](const QString& code) {
        received = code;
    });

    center.requestAction(QStringLiteral("open-settings"));

    EXPECT_EQ(received, QStringLiteral("open-settings"));
}

TEST(NotificationCenterTests, RequestActionIgnoresAnEmptyCode)
{
    NotificationCenter center;
    bool received = false;
    center.actionRequested().onReceive(nullptr, [&received](const QString&) {
        received = true;
    });

    center.requestAction(QString());

    EXPECT_FALSE(received);
}
