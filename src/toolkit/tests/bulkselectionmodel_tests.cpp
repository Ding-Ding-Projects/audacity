/*
* Audacity: A Digital Audio Editor
*/

#include <gtest/gtest.h>

#include "internal/bulkselectionmodel.h"

using namespace au::toolkit;

TEST(BulkSelectionModelTests, StartsEmpty)
{
    BulkSelectionModel model;
    model.setTotalCount(10);
    EXPECT_EQ(model.selectedCount(), 0);
}

TEST(BulkSelectionModelTests, ToggleSelectsAndDeselects)
{
    BulkSelectionModel model;
    model.setTotalCount(10);
    model.toggle(3);
    EXPECT_TRUE(model.isSelected(3));
    EXPECT_EQ(model.selectedCount(), 1);
    model.toggle(3);
    EXPECT_FALSE(model.isSelected(3));
    EXPECT_EQ(model.selectedCount(), 0);
}

TEST(BulkSelectionModelTests, SelectRangeSelectsInclusiveRange)
{
    BulkSelectionModel model;
    model.setTotalCount(10);
    model.selectRange(2, 5);
    EXPECT_EQ(model.selectedCount(), 4);
    EXPECT_TRUE(model.isSelected(2));
    EXPECT_TRUE(model.isSelected(5));
    EXPECT_FALSE(model.isSelected(6));
}

TEST(BulkSelectionModelTests, SelectAllOnPageIsScopedToThatPage)
{
    BulkSelectionModel model;
    model.setTotalCount(20);
    model.selectAllOnPage(0, 4);
    EXPECT_EQ(model.selectedCount(), 5);
    EXPECT_FALSE(model.allMatchesSelected());
}

TEST(BulkSelectionModelTests, SelectAllMatchesCoversEveryRowIncludingFutureTotalChanges)
{
    BulkSelectionModel model;
    model.setTotalCount(20);
    model.selectAllMatches();
    EXPECT_TRUE(model.allMatchesSelected());
    EXPECT_EQ(model.selectedCount(), 20);

    model.setTotalCount(25);
    EXPECT_EQ(model.selectedCount(), 25);
}

TEST(BulkSelectionModelTests, InvertFlipsAnExplicitSelection)
{
    BulkSelectionModel model;
    model.setTotalCount(5);
    model.toggle(0);
    model.toggle(1);
    model.invert();
    EXPECT_EQ(model.selectedCount(), 3);
    EXPECT_FALSE(model.isSelected(0));
    EXPECT_TRUE(model.isSelected(2));
}

TEST(BulkSelectionModelTests, ClearSelectionResetsEverything)
{
    BulkSelectionModel model;
    model.setTotalCount(5);
    model.selectAllMatches();
    model.clearSelection();
    EXPECT_EQ(model.selectedCount(), 0);
    EXPECT_FALSE(model.allMatchesSelected());
}

TEST(BulkSelectionModelTests, SelectedIndexesAreSortedAndDeduplicated)
{
    BulkSelectionModel model;
    model.setTotalCount(5);
    model.toggle(3);
    model.toggle(1);
    model.toggle(3);
    model.toggle(3);

    const QVariantList indexes = model.selectedIndexes();
    ASSERT_EQ(indexes.size(), 2);
    EXPECT_EQ(indexes.at(0).toInt(), 1);
    EXPECT_EQ(indexes.at(1).toInt(), 3);
}
