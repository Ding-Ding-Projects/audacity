/*
* Audacity: A Digital Audio Editor
*/
#include <gtest/gtest.h>

#include "updatebannerstate.h"

using namespace au::squirrelupdate;

TEST(UpdateBannerStateTests, NotWindowsIsAlwaysNotApplicable)
{
    EXPECT_EQ(computeUpdateBannerState(false, true, false, false, false), UpdateBannerState::NotApplicable);
    EXPECT_EQ(computeUpdateBannerState(false, false, true, true, true), UpdateBannerState::NotApplicable);
}

TEST(UpdateBannerStateTests, WindowsButNotASquirrelInstallIsNotApplicable)
{
    EXPECT_EQ(computeUpdateBannerState(true, false, false, false, false), UpdateBannerState::NotApplicable);
}

TEST(UpdateBannerStateTests, CheckingTakesPriorityOverEverythingElse)
{
    EXPECT_EQ(computeUpdateBannerState(true, true, true, true, true), UpdateBannerState::Checking);
    EXPECT_EQ(computeUpdateBannerState(true, true, true, false, false), UpdateBannerState::Checking);
}

TEST(UpdateBannerStateTests, AvailableAndNotCheckingIsReady)
{
    EXPECT_EQ(computeUpdateBannerState(true, true, false, true, false), UpdateBannerState::Ready);
    // An old error is superseded by a package that has since verified.
    EXPECT_EQ(computeUpdateBannerState(true, true, false, true, true), UpdateBannerState::Ready);
}

TEST(UpdateBannerStateTests, AnErrorWithNothingAvailableIsFailed)
{
    EXPECT_EQ(computeUpdateBannerState(true, true, false, false, true), UpdateBannerState::Failed);
}

TEST(UpdateBannerStateTests, TheRestingStateIsNoUpdate)
{
    EXPECT_EQ(computeUpdateBannerState(true, true, false, false, false), UpdateBannerState::NoUpdate);
}
