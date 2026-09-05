/*
* Audacity: A Digital Audio Editor
*/
#include <gtest/gtest.h>

#include "internal/tabstripstate.h"

using namespace au::chronicle;

namespace {
TabStripState sampleState()
{
    TabStripState state;
    state.surfaceId = "main";
    state.dockSide = "top";
    state.collapsed = true;

    TabGroup group;
    group.id = "g1";
    group.name = "Interviews";
    group.color = "#00A0A0";
    state.groups.append(group);

    TabItem home;
    home.id = "home";
    home.title = "Home";
    home.kind = "page";
    home.uri = "audacity://home";
    home.closable = false;
    state.tabs.append(home);

    TabItem pinned;
    pinned.id = "p1";
    pinned.title = "Interview take 1";
    pinned.kind = "project";
    pinned.pinned = true;
    pinned.groupId = "g1";
    state.tabs.append(pinned);

    TabItem second;
    second.id = "p2";
    second.title = "Interview take 2";
    second.kind = "project";
    state.tabs.append(second);

    TabItem panel;
    panel.id = "panel-history";
    panel.title = "History";
    panel.kind = "panel";
    state.tabs.append(panel);

    return state;
}
}

TEST(ChronicleTabStrip, PersistenceRoundTripKeepsOrderPinningGroupsAndCollapse)
{
    const TabStripState original = sampleState();
    const TabStripState restored = TabStripLogic::deserialize(TabStripLogic::serialize(original));

    EXPECT_EQ(restored.surfaceId, original.surfaceId);
    EXPECT_EQ(restored.dockSide, QString("top"));
    EXPECT_TRUE(restored.collapsed);

    ASSERT_EQ(restored.tabs.size(), original.tabs.size());
    for (int i = 0; i < restored.tabs.size(); ++i) {
        EXPECT_EQ(restored.tabs.at(i).id, original.tabs.at(i).id);
        EXPECT_EQ(restored.tabs.at(i).title, original.tabs.at(i).title);
        EXPECT_EQ(restored.tabs.at(i).kind, original.tabs.at(i).kind);
        EXPECT_EQ(restored.tabs.at(i).pinned, original.tabs.at(i).pinned);
        EXPECT_EQ(restored.tabs.at(i).closable, original.tabs.at(i).closable);
        EXPECT_EQ(restored.tabs.at(i).groupId, original.tabs.at(i).groupId);
    }

    ASSERT_EQ(restored.groups.size(), 1);
    EXPECT_EQ(restored.groups.at(0).name, QString("Interviews"));
    EXPECT_EQ(restored.groups.at(0).color, QString("#00A0A0"));
}

TEST(ChronicleTabStrip, AnUnknownDockSideFallsBackToLeft)
{
    TabStripState state = sampleState();
    state.dockSide = "diagonal";
    EXPECT_EQ(TabStripLogic::deserialize(TabStripLogic::serialize(state)).dockSide, QString("left"));
}

TEST(ChronicleTabStrip, DeserializingRubbishGivesAnEmptyStripRatherThanACrash)
{
    EXPECT_TRUE(TabStripLogic::deserialize("not json at all").tabs.isEmpty());
    EXPECT_TRUE(TabStripLogic::deserialize("").tabs.isEmpty());
    EXPECT_EQ(TabStripLogic::deserialize("[1,2,3]").dockSide, QString("left"));
}

TEST(ChronicleTabStrip, CloseTabsContainingText)
{
    const TabStripState state = sampleState();

    const QList<TabItem> victims = TabStripLogic::tabsToClose(state, "interview", false, true, false);

    // Home is not closable and the pinned tab is excluded by default, so only
    // the second interview is taken.
    ASSERT_EQ(victims.size(), 1);
    EXPECT_EQ(victims.at(0).id, QString("p2"));
}

TEST(ChronicleTabStrip, CloseTabsNotContainingTextIsTheExactInverse)
{
    const TabStripState state = sampleState();

    const QList<TabItem> inside = TabStripLogic::tabsToClose(state, "interview", false, true, true);
    const QList<TabItem> outside = TabStripLogic::tabsToClose(state, "interview", false, false, true);

    QStringList insideIds;
    for (const TabItem& tab : inside) {
        insideIds.append(tab.id);
    }
    QStringList outsideIds;
    for (const TabItem& tab : outside) {
        outsideIds.append(tab.id);
    }

    EXPECT_EQ(insideIds, QStringList({ "p1", "p2" }));
    EXPECT_EQ(outsideIds, QStringList({ "panel-history" }));

    // The two sets never overlap and together they cover every closable tab.
    for (const QString& id : insideIds) {
        EXPECT_FALSE(outsideIds.contains(id));
    }
    EXPECT_EQ(insideIds.size() + outsideIds.size(), 3);
}

TEST(ChronicleTabStrip, PinnedTabsAreExcludedUnlessAskedFor)
{
    const TabStripState state = sampleState();

    EXPECT_EQ(TabStripLogic::tabsToClose(state, "take 1", false, true, false).size(), 0);
    EXPECT_EQ(TabStripLogic::tabsToClose(state, "take 1", false, true, true).size(), 1);
}

TEST(ChronicleTabStrip, AnEmptyOrBrokenQueryClosesNothing)
{
    const TabStripState state = sampleState();

    EXPECT_TRUE(TabStripLogic::tabsToClose(state, "", false, true, true).isEmpty());
    EXPECT_TRUE(TabStripLogic::tabsToClose(state, "", false, false, true).isEmpty());
    EXPECT_TRUE(TabStripLogic::tabsToClose(state, "interview(", true, true, true).isEmpty());
    EXPECT_TRUE(TabStripLogic::tabsToClose(state, "interview(", true, false, true).isEmpty());
}

TEST(ChronicleTabStrip, RegexMatchingIsUsedOnlyWhenAskedFor)
{
    const TabStripState state = sampleState();

    // As plain text, the pattern matches nothing.
    EXPECT_TRUE(TabStripLogic::tabsToClose(state, "take [0-9]", false, true, true).isEmpty());
    // As a regular expression, it matches both interviews.
    EXPECT_EQ(TabStripLogic::tabsToClose(state, "take [0-9]", true, true, true).size(), 2);

    EXPECT_TRUE(TabStripLogic::isValidRegex("take [0-9]"));
    EXPECT_FALSE(TabStripLogic::isValidRegex("take ["));
    EXPECT_FALSE(TabStripLogic::isValidRegex(""));
}

TEST(ChronicleTabStrip, MatchingIsCaseInsensitive)
{
    EXPECT_TRUE(TabStripLogic::matches("Interview take 1", "INTERVIEW", false));
    EXPECT_TRUE(TabStripLogic::matches("Interview take 1", "^interview", true));
    EXPECT_FALSE(TabStripLogic::matches("Interview take 1", "", false));
}
