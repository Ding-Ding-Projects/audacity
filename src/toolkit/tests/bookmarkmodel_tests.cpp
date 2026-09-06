/*
* Audacity: A Digital Audio Editor
*/

#include <gtest/gtest.h>

#include "internal/bookmarkmodel.h"

using namespace au::toolkit;

TEST(BookmarkModelTests, StartsEmpty)
{
    BookmarkModel model;
    EXPECT_EQ(model.rowCount(), 0);
}

TEST(BookmarkModelTests, AddInsertsARowAndMarksTheArticleBookmarked)
{
    BookmarkModel model;
    model.add("appearance-editor", "Appearance editor");
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_TRUE(model.isBookmarked("appearance-editor"));
    EXPECT_FALSE(model.isBookmarked("docs-browser"));
}

TEST(BookmarkModelTests, AddIsIdempotentForTheSameArticle)
{
    BookmarkModel model;
    model.add("appearance-editor", "Appearance editor");
    model.add("appearance-editor", "Appearance editor again");
    EXPECT_EQ(model.rowCount(), 1);
}

TEST(BookmarkModelTests, RemoveByArticleIdDropsExactlyThatRow)
{
    BookmarkModel model;
    model.add("a", "A");
    model.add("b", "B");
    model.removeByArticleId("a");
    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_FALSE(model.isBookmarked("a"));
    EXPECT_TRUE(model.isBookmarked("b"));
}

TEST(BookmarkModelTests, ToggleAddsThenRemoves)
{
    BookmarkModel model;
    model.toggle("a", "A");
    EXPECT_TRUE(model.isBookmarked("a"));
    model.toggle("a", "A");
    EXPECT_FALSE(model.isBookmarked("a"));
}

TEST(BookmarkModelTests, RenameChangesOnlyTheDisplayedTitle)
{
    BookmarkModel model;
    model.add("a", "Original");
    model.rename("a", "Renamed");

    const QModelIndex idx = model.index(0);
    EXPECT_EQ(model.data(idx, BookmarkModel::TitleRole).toString(), QStringLiteral("Renamed"));
    EXPECT_EQ(model.data(idx, BookmarkModel::ArticleIdRole).toString(), QStringLiteral("a"));
}

TEST(BookmarkModelTests, RemoveManyDropsEveryNamedRowRegardlessOfOrder)
{
    BookmarkModel model;
    model.add("a", "A");
    model.add("b", "B");
    model.add("c", "C");

    QVariantList rows;
    rows.append(0);
    rows.append(2);
    model.removeMany(rows);

    EXPECT_EQ(model.rowCount(), 1);
    EXPECT_TRUE(model.isBookmarked("b"));
}

TEST(BookmarkModelTests, ExportRowsCarryIdArticleIdAndTitle)
{
    BookmarkModel model;
    model.add("a", "A");

    const QVariantList rows = model.toExportRows();
    ASSERT_EQ(rows.size(), 1);
    const QVariantMap row = rows.at(0).toMap();
    EXPECT_EQ(row.value("articleId").toString(), QStringLiteral("a"));
    EXPECT_EQ(row.value("title").toString(), QStringLiteral("A"));
    EXPECT_FALSE(row.value("id").toString().isEmpty());
}
