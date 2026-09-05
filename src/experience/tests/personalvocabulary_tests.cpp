/*
 * Audacity: A Digital Audio Editor
 */
#include <gtest/gtest.h>

#include "internal/personalvocabulary.h"

using namespace au::experience;

TEST(PersonalVocabularyTests, ReadsAValidFile)
{
    const QByteArray data = R"({"version":1,"entries":[{"from":"track","to":"lane"},{"from":"clip","to":"take"}]})";
    const PersonalVocabulary::ParseResult result = PersonalVocabulary::parse(data);

    ASSERT_TRUE(result.ok) << result.error.toStdString();
    EXPECT_EQ(result.entries.size(), 2);
}

TEST(PersonalVocabularyTests, RejectsAnEmptyFile)
{
    EXPECT_FALSE(PersonalVocabulary::parse(QByteArray()).ok);
}

TEST(PersonalVocabularyTests, RejectsBrokenJson)
{
    EXPECT_FALSE(PersonalVocabulary::parse(QByteArray("{ not json")).ok);
}

TEST(PersonalVocabularyTests, RejectsAnUnknownVersion)
{
    const QByteArray data = R"({"version":2,"entries":[{"from":"a","to":"b"}]})";
    EXPECT_FALSE(PersonalVocabulary::parse(data).ok);
}

TEST(PersonalVocabularyTests, RejectsAMissingEntriesArray)
{
    EXPECT_FALSE(PersonalVocabulary::parse(QByteArray(R"({"version":1})")).ok);
}

TEST(PersonalVocabularyTests, RejectsAnEntryWithoutStrings)
{
    const QByteArray data = R"({"version":1,"entries":[{"from":"a","to":4}]})";
    EXPECT_FALSE(PersonalVocabulary::parse(data).ok);
}

TEST(PersonalVocabularyTests, RejectsAnEmptyFrom)
{
    const QByteArray data = R"({"version":1,"entries":[{"from":"","to":"b"}]})";
    EXPECT_FALSE(PersonalVocabulary::parse(data).ok);
}

TEST(PersonalVocabularyTests, RejectsARepeatedFrom)
{
    const QByteArray data = R"({"version":1,"entries":[{"from":"a","to":"b"},{"from":"a","to":"c"}]})";
    EXPECT_FALSE(PersonalVocabulary::parse(data).ok);
}

TEST(PersonalVocabularyTests, AcceptsExactlyTheEntryLimit)
{
    QByteArray data = R"({"version":1,"entries":[)";
    for (int i = 0; i < PersonalVocabulary::MAX_ENTRIES; ++i) {
        if (i > 0) {
            data += ",";
        }
        data += QStringLiteral(R"({"from":"term%1","to":"word%1"})").arg(i).toUtf8();
    }
    data += "]}";

    const PersonalVocabulary::ParseResult result = PersonalVocabulary::parse(data);
    ASSERT_TRUE(result.ok) << result.error.toStdString();
    EXPECT_EQ(result.entries.size(), PersonalVocabulary::MAX_ENTRIES);
}

TEST(PersonalVocabularyTests, RejectsOneEntryTooMany)
{
    QByteArray data = R"({"version":1,"entries":[)";
    for (int i = 0; i <= PersonalVocabulary::MAX_ENTRIES; ++i) {
        if (i > 0) {
            data += ",";
        }
        data += QStringLiteral(R"({"from":"term%1","to":"word%1"})").arg(i).toUtf8();
    }
    data += "]}";

    EXPECT_FALSE(PersonalVocabulary::parse(data).ok);
}

TEST(PersonalVocabularyTests, RejectsAFileOverTheSizeLimit)
{
    QByteArray data(PersonalVocabulary::MAX_BYTES + 1, 'x');
    EXPECT_FALSE(PersonalVocabulary::parse(data).ok);
}

TEST(PersonalVocabularyTests, SubstitutesWholeWordsOnly)
{
    PersonalVocabulary::Table table { { QStringLiteral("track"), QStringLiteral("lane") } };

    EXPECT_EQ(PersonalVocabulary::apply(QStringLiteral("Add a track"), table), QStringLiteral("Add a lane"));
    EXPECT_EQ(PersonalVocabulary::apply(QStringLiteral("Tracking is fine"), table), QStringLiteral("Tracking is fine"));
    EXPECT_EQ(PersonalVocabulary::apply(QStringLiteral("backtrack"), table), QStringLiteral("backtrack"));
    EXPECT_EQ(PersonalVocabulary::apply(QStringLiteral("track, track."), table), QStringLiteral("lane, lane."));
}

TEST(PersonalVocabularyTests, LongerTermsWinOverShorterOnes)
{
    const QByteArray data = R"({"version":1,"entries":[{"from":"audio","to":"sound"},{"from":"audio track","to":"sound lane"}]})";
    const PersonalVocabulary::ParseResult result = PersonalVocabulary::parse(data);
    ASSERT_TRUE(result.ok);

    EXPECT_EQ(PersonalVocabulary::apply(QStringLiteral("audio track"), result.entries), QStringLiteral("sound lane"));
}

TEST(PersonalVocabularyTests, SerialisesBackToTheSameTable)
{
    const QByteArray data = R"({"version":1,"entries":[{"from":"track","to":"lane"}]})";
    const PersonalVocabulary::ParseResult first = PersonalVocabulary::parse(data);
    ASSERT_TRUE(first.ok);

    const PersonalVocabulary::ParseResult second = PersonalVocabulary::parse(PersonalVocabulary::serialize(first.entries));
    ASSERT_TRUE(second.ok);
    EXPECT_EQ(second.entries, first.entries);
}
