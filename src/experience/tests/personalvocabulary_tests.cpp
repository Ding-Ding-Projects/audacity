/*
 * Audacity: A Digital Audio Editor
 */
#include <gtest/gtest.h>

#include "internal/personalvocabulary.h"

using namespace au::experience;

TEST(PersonalVocabularyTests, ReadsAValidFile)
{
    const QByteArray data = R"({"schemaVersion":1,"entries":{"track":"lane","clip":"take"}})";
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

TEST(PersonalVocabularyTests, RejectsLegacyShapeForImports)
{
    const QByteArray data = R"({"version":1,"entries":[{"from":"a","to":"b"}]})";
    EXPECT_FALSE(PersonalVocabulary::parse(data).ok);
}

TEST(PersonalVocabularyTests, MigratesAValidatedLegacyCache)
{
    const QByteArray data = R"({"version":1,"entries":[{"from":"track","to":"lane"},{"from":"clip","to":"take"}]})";
    const PersonalVocabulary::ParseResult result = PersonalVocabulary::parseStoredCache(data);

    ASSERT_TRUE(result.ok) << result.error.toStdString();
    EXPECT_TRUE(result.migratedLegacy);
    EXPECT_EQ(result.entries.size(), 2);
}

TEST(PersonalVocabularyTests, RejectsInvalidLegacyCacheEntries)
{
    const QByteArray data = R"({"version":1,"entries":[{"from":"a","to":4}]})";
    EXPECT_FALSE(PersonalVocabulary::parseStoredCache(data).ok);
}

TEST(PersonalVocabularyTests, RejectsARepeatedLegacySource)
{
    const QByteArray data = R"({"version":1,"entries":[{"from":"a","to":"b"},{"from":"a","to":"c"}]})";
    EXPECT_FALSE(PersonalVocabulary::parseStoredCache(data).ok);
}

TEST(PersonalVocabularyTests, RejectsDuplicateDecodedCanonicalKeys)
{
    const QByteArray data = R"({"schemaVersion":1,"entries":{"a":"b","\u0061":"c"}})";
    EXPECT_FALSE(PersonalVocabulary::parse(data).ok);
}

TEST(PersonalVocabularyTests, RejectsUnsafeAndControlEntryKeys)
{
    EXPECT_FALSE(PersonalVocabulary::parse(QByteArray(R"({"schemaVersion":1,"entries":{"__proto__":"value"}})")).ok);
    EXPECT_FALSE(PersonalVocabulary::parse(QByteArray(R"({"schemaVersion":1,"entries":{"a\u0001":"value"}})")).ok);
}

TEST(PersonalVocabularyTests, RejectsExcessiveJsonDepth)
{
    QByteArray value = R"("value")";
    for (int i = 0; i < 8; ++i) {
        value = QByteArray("[") + value + QByteArray("]");
    }
    const QByteArray data = QByteArray(R"({"schemaVersion":1,"entries":{"term":)") + value + QByteArray("}}");
    EXPECT_FALSE(PersonalVocabulary::parse(data).ok);
}

TEST(PersonalVocabularyTests, AcceptsAnEmptyCanonicalTable)
{
    const PersonalVocabulary::ParseResult result = PersonalVocabulary::parse(QByteArray(R"({"schemaVersion":1,"entries":{}})"));
    ASSERT_TRUE(result.ok) << result.error.toStdString();
    EXPECT_TRUE(result.entries.isEmpty());
}

TEST(PersonalVocabularyTests, AcceptsExactlyTheEntryLimit)
{
    QByteArray data = R"({"schemaVersion":1,"entries":{)";
    for (int i = 0; i < PersonalVocabulary::MAX_ENTRIES; ++i) {
        if (i > 0) {
            data += ",";
        }
        data += QStringLiteral(R"("term%1":"word%1")").arg(i).toUtf8();
    }
    data += "}}";

    const PersonalVocabulary::ParseResult result = PersonalVocabulary::parse(data);
    ASSERT_TRUE(result.ok) << result.error.toStdString();
    EXPECT_EQ(result.entries.size(), PersonalVocabulary::MAX_ENTRIES);
    const PersonalVocabulary::MatcherPtr matcher = PersonalVocabulary::compile(result.entries);
    ASSERT_TRUE(matcher);
    EXPECT_EQ(PersonalVocabulary::apply(QStringLiteral("term4095"), matcher), QStringLiteral("word4095"));
}

TEST(PersonalVocabularyTests, RejectsOneEntryTooMany)
{
    QByteArray data = R"({"schemaVersion":1,"entries":{)";
    for (int i = 0; i <= PersonalVocabulary::MAX_ENTRIES; ++i) {
        if (i > 0) {
            data += ",";
        }
        data += QStringLiteral(R"("term%1":"word%1")").arg(i).toUtf8();
    }
    data += "}}";

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
    const QByteArray data = R"({"schemaVersion":1,"entries":{"audio":"sound","audio track":"sound lane"}})";
    const PersonalVocabulary::ParseResult result = PersonalVocabulary::parse(data);
    ASSERT_TRUE(result.ok);

    EXPECT_EQ(PersonalVocabulary::apply(QStringLiteral("audio track"), result.entries), QStringLiteral("sound lane"));
}

TEST(PersonalVocabularyTests, SubstitutesFromTheOriginalTextOnly)
{
    const PersonalVocabulary::Table table {
        { QStringLiteral("Audio"), QStringLiteral("Sound") },
        { QStringLiteral("Sound"), QStringLiteral("Noise") },
    };

    EXPECT_EQ(PersonalVocabulary::apply(QStringLiteral("Audio Sound"), table), QStringLiteral("Sound Noise"));
}

TEST(PersonalVocabularyTests, UsesTheLongestOverlappingPhraseAtOnePosition)
{
    const PersonalVocabulary::Table table {
        { QStringLiteral("audio"), QStringLiteral("sound") },
        { QStringLiteral("audio track"), QStringLiteral("sound lane") },
    };

    EXPECT_EQ(PersonalVocabulary::apply(QStringLiteral("audio track audio"), table), QStringLiteral("sound lane sound"));
}

TEST(PersonalVocabularyTests, MatchesChineseTermsLiterallyWithoutAsciiWordBoundaries)
{
    const PersonalVocabulary::Table table { { QStringLiteral("軌"), QStringLiteral("線") } };

    EXPECT_EQ(PersonalVocabulary::apply(QStringLiteral("A軌B"), table), QStringLiteral("A線B"));
}

TEST(PersonalVocabularyTests, SerialisesBackToTheSameTable)
{
    const QByteArray data = R"({"schemaVersion":1,"entries":{"track":"lane"}})";
    const PersonalVocabulary::ParseResult first = PersonalVocabulary::parse(data);
    ASSERT_TRUE(first.ok);

    const PersonalVocabulary::ParseResult second = PersonalVocabulary::parse(PersonalVocabulary::serialize(first.entries));
    ASSERT_TRUE(second.ok);
    EXPECT_EQ(second.entries, first.entries);
}
