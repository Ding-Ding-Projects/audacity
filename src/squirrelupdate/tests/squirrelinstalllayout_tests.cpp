/*
* Audacity: A Digital Audio Editor
*/
#include <gtest/gtest.h>

#include "internal/squirrelinstalllayout.h"

using namespace au::squirrelupdate;

TEST(SquirrelInstallLayoutTests, VersionFromPathReadsTheAppDirectory)
{
    const QString path = "C:/Users/x/AppData/Local/Audacity/app-4.0.0-m3001/bin";
    EXPECT_EQ(SquirrelInstallLayout::versionFromPath(path), "4.0.0-m3001");
}

TEST(SquirrelInstallLayoutTests, VersionFromPathIsEmptyOutsideASquirrelInstall)
{
    EXPECT_TRUE(SquirrelInstallLayout::versionFromPath("C:/Program Files/Audacity/bin").isEmpty());
}

TEST(SquirrelInstallLayoutTests, AppDirFromPathStopsAtTheAppSegment)
{
    const QString path = "C:/Users/x/AppData/Local/Audacity/app-4.0.0-m3001/bin/nested";
    EXPECT_EQ(SquirrelInstallLayout::appDirFromPath(path), "C:/Users/x/AppData/Local/Audacity/app-4.0.0-m3001");
}

TEST(SquirrelInstallLayoutTests, RootDirFromPathIsTheParentOfTheAppDirectory)
{
    const QString path = "C:/Users/x/AppData/Local/Audacity/app-4.0.0-m3001/bin";
    EXPECT_EQ(SquirrelInstallLayout::rootDirFromPath(path), "C:/Users/x/AppData/Local/Audacity");
}

TEST(SquirrelInstallLayoutTests, RootDirFromPathIsEmptyWhenThereIsNoAppSegment)
{
    EXPECT_TRUE(SquirrelInstallLayout::rootDirFromPath("C:/somewhere/else").isEmpty());
}
