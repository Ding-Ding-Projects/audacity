/*
* Audacity: A Digital Audio Editor
*/
#include <gtest/gtest.h>

#include "internal/changelogparser.h"
#include "view/changelogmodel.h"

using namespace au::chronicle;

static const QString SAMPLE = QStringLiteral(
    "# Changelog\n"
    "\n"
    "Some prose that is not part of any release.\n"
    "\n"
    "## 4.0.0-material.1 - 2026-09-05\n"
    "\n"
    "### Added\n"
    "\n"
    "- Material 3 token engine "
    "([`9e500daf17`](https://github.com/Ding-Ding-Projects/audacity/commit/"
    "9e500daf17c1f55f928c92d84090a335be253c97))\n"
    "- Material 3 app shell "
    "([`bacb4d8352`](https://github.com/Ding-Ding-Projects/audacity/commit/"
    "bacb4d8352ab18fcda449d11abad44565ad68aae))\n"
    "\n"
    "### Documentation\n"
    "\n"
    "- Add the documentation site "
    "([`ddea8d276f`](https://github.com/Ding-Ding-Projects/audacity/commit/"
    "ddea8d276f4ce203fd8c4f69118f02e1412285f9))\n"
    "\n"
    "## 3.9.0 - 2026-01-02\n"
    "\n"
    "### Changed\n"
    "\n"
    "- Something older "
    "([`0a5af7b198`](https://github.com/Ding-Ding-Projects/audacity/commit/"
    "0a5af7b198eec2737ffeef5f3facb8dedadd7670))\n");

TEST(ChronicleChangelog, ParsesReleasesGroupsAndCommitHashes)
{
    const QList<ChangelogRelease> releases = ChangelogParser::parse(SAMPLE);

    ASSERT_EQ(releases.size(), 2);

    EXPECT_EQ(releases.at(0).version, QString("4.0.0-material.1"));
    EXPECT_EQ(releases.at(0).date, QDate(2026, 9, 5));
    ASSERT_EQ(releases.at(0).entries.size(), 3);

    const ChangelogEntry& first = releases.at(0).entries.at(0);
    EXPECT_EQ(first.group, QString("Added"));
    EXPECT_EQ(first.text, QString("Material 3 token engine"));
    // The full hash is what is kept, not the abbreviation shown in the link.
    EXPECT_EQ(first.commitSha, QString("9e500daf17c1f55f928c92d84090a335be253c97"));

    EXPECT_EQ(releases.at(0).entries.at(2).group, QString("Documentation"));
    EXPECT_EQ(releases.at(1).version, QString("3.9.0"));
    ASSERT_EQ(releases.at(1).entries.size(), 1);
}

TEST(ChronicleChangelog, EveryEntryCarriesAFullCommitHash)
{
    for (const ChangelogRelease& release : ChangelogParser::parse(SAMPLE)) {
        for (const ChangelogEntry& entry : release.entries) {
            EXPECT_EQ(entry.commitSha.size(), 40) << entry.text.toStdString();
        }
    }
}

TEST(ChronicleChangelog, CollectsEveryHashForTheBuildTimeCheck)
{
    const QStringList hashes = ChangelogParser::commitShas(SAMPLE);
    ASSERT_EQ(hashes.size(), 4);
    EXPECT_TRUE(hashes.contains(QString("bacb4d8352ab18fcda449d11abad44565ad68aae")));
    // The check must not be fooled by the abbreviation in the link text.
    for (const QString& hash : hashes) {
        EXPECT_EQ(hash.size(), 40);
    }
}

TEST(ChronicleChangelog, CommitUrlPointsAtThePublicRepository)
{
    EXPECT_EQ(ChangelogParser::commitUrl("9e500daf17c1f55f928c92d84090a335be253c97"),
              QString("https://github.com/Ding-Ding-Projects/audacity/commit/"
                      "9e500daf17c1f55f928c92d84090a335be253c97"));
}

TEST(ChronicleChangelog, FiltersByDateAndBySearchTerm)
{
    ChangelogModel model;
    model.loadText(SAMPLE);

    EXPECT_EQ(model.releases().size(), 2);

    model.setFromDate("2026-06-01");
    EXPECT_EQ(model.releases().size(), 1);

    model.clearFilters();
    model.setSearchText("app shell");
    ASSERT_EQ(model.releases().size(), 1);
    EXPECT_EQ(model.releases().at(0).toMap().value("entries").toList().size(), 1);

    // A term that is not a valid regular expression falls back to plain text
    // rather than emptying the list.
    model.setSearchText("engine[");
    EXPECT_EQ(model.releases().size(), 0);
}

TEST(ChronicleChangelog, ExportsMarkdownJsonAndHtmlWithFullHashes)
{
    ChangelogModel model;
    model.loadText(SAMPLE);

    const QString markdown = model.exportText("markdown");
    EXPECT_TRUE(markdown.contains("9e500daf17c1f55f928c92d84090a335be253c97"));
    EXPECT_TRUE(markdown.contains("## 4.0.0-material.1 - 2026-09-05"));

    const QString json = model.exportText("json");
    EXPECT_TRUE(json.contains("\"commit\""));
    EXPECT_TRUE(json.contains("9e500daf17c1f55f928c92d84090a335be253c97"));

    const QString html = model.exportText("html");
    EXPECT_TRUE(html.contains("<h2>4.0.0-material.1 - 2026-09-05</h2>"));
    EXPECT_TRUE(html.contains("https://github.com/Ding-Ding-Projects/audacity/commit/"
                              "9e500daf17c1f55f928c92d84090a335be253c97"));
}
