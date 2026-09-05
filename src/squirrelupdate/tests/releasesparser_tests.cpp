/*
* Audacity: A Digital Audio Editor
*/
#include <gtest/gtest.h>

#include "internal/releasesparser.h"

using namespace au::squirrelupdate;

TEST(ReleasesParserTests, ParsesAWellFormedLine)
{
    const QString sha1 = QString(40, QChar('a'));
    const QString line = sha1 + " Audacity-4.0.0-m3001-full.nupkg 12345";

    QString error;
    const ReleaseEntry entry = ReleasesParser::parseLine(line, &error);

    ASSERT_TRUE(entry.isValid()) << error.toStdString();
    EXPECT_EQ(entry.sha1, sha1.toUpper());
    EXPECT_EQ(entry.fileName, QStringLiteral("Audacity-4.0.0-m3001-full.nupkg"));
    EXPECT_EQ(entry.size, 12345);
    EXPECT_EQ(entry.version, QStringLiteral("4.0.0-m3001"));
    EXPECT_FALSE(entry.isDelta);
}

TEST(ReleasesParserTests, DetectsADeltaPackage)
{
    const QString sha1 = QString(40, QChar('b'));
    const QString line = sha1 + " Audacity-4.0.0-m3002-delta.nupkg 999";

    const ReleaseEntry entry = ReleasesParser::parseLine(line);

    ASSERT_TRUE(entry.isValid());
    EXPECT_TRUE(entry.isDelta);
}

TEST(ReleasesParserTests, BlankAndCommentLinesProduceNoError)
{
    QString error;
    EXPECT_FALSE(ReleasesParser::parseLine("", &error).isValid());
    EXPECT_TRUE(error.isEmpty());

    EXPECT_FALSE(ReleasesParser::parseLine("   ", &error).isValid());
    EXPECT_TRUE(error.isEmpty());

    EXPECT_FALSE(ReleasesParser::parseLine("# a comment", &error).isValid());
    EXPECT_TRUE(error.isEmpty());
}

TEST(ReleasesParserTests, RejectsAShortHash)
{
    QString error;
    const ReleaseEntry entry = ReleasesParser::parseLine("abc Audacity-4.0.0-full.nupkg 10", &error);

    EXPECT_FALSE(entry.isValid());
    EXPECT_FALSE(error.isEmpty());
}

TEST(ReleasesParserTests, RejectsAFileThatIsNotANupkg)
{
    const QString sha1 = QString(40, QChar('c'));
    QString error;
    const ReleaseEntry entry = ReleasesParser::parseLine(sha1 + " Audacity-4.0.0-full.zip 10", &error);

    EXPECT_FALSE(entry.isValid());
    EXPECT_FALSE(error.isEmpty());
}

TEST(ReleasesParserTests, RejectsANonPositiveSize)
{
    const QString sha1 = QString(40, QChar('d'));
    QString error;
    const ReleaseEntry entry = ReleasesParser::parseLine(sha1 + " Audacity-4.0.0-full.nupkg 0", &error);

    EXPECT_FALSE(entry.isValid());
    EXPECT_FALSE(error.isEmpty());
}

TEST(ReleasesParserTests, RejectsTheWrongFieldCount)
{
    QString error;
    const ReleaseEntry entry = ReleasesParser::parseLine("only two fields", &error);

    EXPECT_FALSE(entry.isValid());
    EXPECT_FALSE(error.isEmpty());
}

TEST(ReleasesParserTests, ParseSkipsMalformedLinesAndKeepsGoodOnes)
{
    const QString goodSha = QString(40, QChar('e'));
    const QString text = "garbage line\n" + goodSha + " Audacity-4.0.1-full.nupkg 555\n\n# comment";

    QStringList errors;
    const ReleaseEntryList entries = ReleasesParser::parse(text, &errors);

    ASSERT_EQ(entries.size(), 1);
    EXPECT_EQ(entries.first().version, QStringLiteral("4.0.1"));
    EXPECT_FALSE(errors.isEmpty());
}

TEST(ReleasesParserTests, VersionFromFileNameStripsSuffixAndPath)
{
    EXPECT_EQ(ReleasesParser::versionFromFileName("Audacity-4.0.0-m3001-full.nupkg"), "4.0.0-m3001");
    EXPECT_EQ(ReleasesParser::versionFromFileName("https://example.test/x/Audacity-4.2.0-delta.nupkg"), "4.2.0");
    EXPECT_TRUE(ReleasesParser::versionFromFileName("Audacity.nupkg").isEmpty());
}

TEST(ReleasesParserTests, CompareVersionsOrdersNumericallyAndByPreRelease)
{
    EXPECT_LT(ReleasesParser::compareVersions("4.0.0", "4.0.1"), 0);
    EXPECT_GT(ReleasesParser::compareVersions("4.1.0", "4.0.9"), 0);
    EXPECT_EQ(ReleasesParser::compareVersions("4.0.0", "4.0.0"), 0);

    // No pre-release label outranks any label.
    EXPECT_GT(ReleasesParser::compareVersions("4.0.0", "4.0.0-m3001"), 0);
    EXPECT_LT(ReleasesParser::compareVersions("4.0.0-m3001", "4.0.0-m3002"), 0);
}

TEST(ReleasesParserTests, NewestPrefersFullOverDeltaAtTheSameVersion)
{
    const QString sha1 = QString(40, QChar('f'));
    ReleaseEntry delta = ReleasesParser::parseLine(sha1 + " Audacity-4.0.0-delta.nupkg 10");
    ReleaseEntry full = ReleasesParser::parseLine(sha1 + " Audacity-4.0.0-full.nupkg 20");

    const ReleaseEntry newest = ReleasesParser::newest({ delta, full });

    EXPECT_FALSE(newest.isDelta);
    EXPECT_EQ(newest.fileName, full.fileName);
}

TEST(ReleasesParserTests, NewestAfterOnlyReturnsStrictlyNewerEntries)
{
    ReleaseEntry older = ReleasesParser::parseLine(QString(40, QChar('1')) + " Audacity-3.9.0-full.nupkg 10");
    ReleaseEntry same = ReleasesParser::parseLine(QString(40, QChar('2')) + " Audacity-4.0.0-full.nupkg 10");
    ReleaseEntry newer = ReleasesParser::parseLine(QString(40, QChar('3')) + " Audacity-4.1.0-full.nupkg 10");

    const ReleaseEntry result = ReleasesParser::newestAfter({ older, same, newer }, "4.0.0");

    EXPECT_EQ(result.version, "4.1.0");
}

TEST(ReleasesParserTests, NewestAfterReturnsInvalidWhenNothingIsNewer)
{
    ReleaseEntry same = ReleasesParser::parseLine(QString(40, QChar('4')) + " Audacity-4.0.0-full.nupkg 10");

    const ReleaseEntry result = ReleasesParser::newestAfter({ same }, "4.0.0");

    EXPECT_FALSE(result.isValid());
}
