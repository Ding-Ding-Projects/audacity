/*
 * Audacity: A Digital Audio Editor
 */
#include <gtest/gtest.h>

#include "internal/narratorqueue.h"

using namespace au::experience;

namespace {
NarratorUtterance makeUtterance(const QString& text, NarratorCategory category = NarratorCategory::General,
                                const QString& supersedeKey = QString())
{
    NarratorUtterance utterance;
    utterance.text = text;
    utterance.category = category;
    utterance.supersedeKey = supersedeKey;
    return utterance;
}
}

TEST(NarratorQueueTests, QueuesAndPopsInOrder)
{
    NarratorQueue queue;
    EXPECT_TRUE(queue.enqueue(makeUtterance(QStringLiteral("Recording started")), 0));
    EXPECT_TRUE(queue.enqueue(makeUtterance(QStringLiteral("Track added")), 10000));

    EXPECT_EQ(queue.popNext().text, QStringLiteral("Recording started"));
    EXPECT_EQ(queue.popNext().text, QStringLiteral("Track added"));
    EXPECT_TRUE(queue.isEmpty());
}

TEST(NarratorQueueTests, DebouncesTheSameTextArrivingQuickly)
{
    NarratorQueue queue(400, 4000);
    EXPECT_TRUE(queue.enqueue(makeUtterance(QStringLiteral("Saved")), 0));
    EXPECT_FALSE(queue.enqueue(makeUtterance(QStringLiteral("Saved")), 100));
    EXPECT_TRUE(queue.enqueue(makeUtterance(QStringLiteral("Saved")), 5000));
}

TEST(NarratorQueueTests, CoolsDownACategoryButNeverAnError)
{
    NarratorQueue queue(400, 4000);
    EXPECT_TRUE(queue.enqueue(makeUtterance(QStringLiteral("Saved")), 0));
    EXPECT_FALSE(queue.enqueue(makeUtterance(QStringLiteral("Autosaved")), 500));
    EXPECT_TRUE(queue.enqueue(makeUtterance(QStringLiteral("Disk is full"), NarratorCategory::Error), 500));
}

TEST(NarratorQueueTests, SupersedesAPendingItemWithTheSameKeyInstead)
{
    NarratorQueue queue;
    queue.enqueue(makeUtterance(QStringLiteral("Exporting: 10%"), NarratorCategory::General,
                                QStringLiteral("export-progress")), 0);
    queue.enqueue(makeUtterance(QStringLiteral("Exporting: 50%"), NarratorCategory::General,
                                QStringLiteral("export-progress")), 10000);

    EXPECT_EQ(queue.size(), 1);
    EXPECT_EQ(queue.popNext().text, QStringLiteral("Exporting: 50%"));
}

TEST(NarratorQueueTests, OneAtATimeNeverOverlaps)
{
    NarratorQueue queue;
    queue.enqueue(makeUtterance(QStringLiteral("A")), 0);
    queue.enqueue(makeUtterance(QStringLiteral("B")), 20000);

    ASSERT_EQ(queue.size(), 2);
    const NarratorUtterance first = queue.popNext();
    EXPECT_EQ(queue.size(), 1);
    EXPECT_EQ(first.text, QStringLiteral("A"));
}
