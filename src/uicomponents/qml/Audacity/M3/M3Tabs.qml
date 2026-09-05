/*
* Audacity: A Digital Audio Editor
*
* M3Tabs
*
* A Material 3 tab strip, primary or secondary, horizontal or vertical. The
* vertical orientation suits a dockable panel strip along the side of the
* window. The strip owns one NavigationPanel so the arrow keys move between
* tabs and the tab key leaves the strip.
*
* Replaces: Muse.UiComponents StyledTabBar.
*
* API:
*     model (list of { text, icon, badgeCount, name, visible }), currentIndex,
*     primary, orientation, activated(index), navigationPanel, currentItem
*
* A model entry may name its navigation control and may hide itself, so that a
* strip whose tabs appear conditionally keeps stable indexes.
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Item {
    id: root

    property var model: []
    property int currentIndex: 0
    property bool primary: true
    property int orientation: Qt.Horizontal

    property NavigationPanel navigationPanel: null

    signal activated(int index)

    readonly property bool vertical: root.orientation === Qt.Vertical

    // The tab that is currently selected, for callers that read its navigation
    // control when the strip becomes active.
    readonly property Item currentItem: repeater.count > root.currentIndex && root.currentIndex >= 0 ? repeater.itemAt(root.currentIndex) as Item : null

    readonly property int visibleCount: {
        if (!root.model) {
            return 0
        }
        var count = 0
        for (var i = 0; i < root.model.length; ++i) {
            var entry = root.model[i]
            if (typeof entry !== "object" || entry.visible !== false) {
                ++count
            }
        }
        return count
    }

    implicitHeight: root.vertical ? repeaterColumn.implicitHeight : 48
    implicitWidth: root.vertical ? 200 : repeaterColumn.implicitWidth

    Rectangle {
        anchors.fill: parent
        color: M3.color.surface
    }

    Grid {
        id: repeaterColumn

        columns: root.vertical ? 1 : repeater.count
        rows: root.vertical ? repeater.count : 1
        anchors.fill: parent

        Repeater {
            id: repeater

            model: root.model

            delegate: M3Tab {
                id: tab

                required property int index
                required property var modelData

                width: root.vertical ? root.width : root.width / Math.max(1, root.visibleCount)
                height: root.vertical ? 48 : root.height

                visible: typeof tab.modelData !== "object" || tab.modelData.visible !== false

                primary: root.primary
                orientation: root.orientation
                selected: root.currentIndex === tab.index

                text: typeof tab.modelData === "string" ? tab.modelData : (tab.modelData.text !== undefined ? tab.modelData.text : "")
                icon: typeof tab.modelData === "object" && tab.modelData.icon !== undefined ? tab.modelData.icon : IconCode.NONE
                badgeCount: typeof tab.modelData === "object" && tab.modelData.badgeCount !== undefined ? tab.modelData.badgeCount : 0

                navigation.panel: root.navigationPanel
                navigation.name: typeof tab.modelData === "object" && tab.modelData.name !== undefined ? String(tab.modelData.name) : tab.text
                navigation.row: root.vertical ? tab.index : 0
                navigation.column: root.vertical ? 0 : tab.index

                onClicked: {
                    root.currentIndex = tab.index
                    root.activated(tab.index)
                }
            }
        }
    }

    M3Divider {
        visible: !root.vertical
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }

    M3Divider {
        visible: root.vertical
        orientation: Qt.Vertical
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }
}
