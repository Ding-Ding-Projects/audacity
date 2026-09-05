/*
* Audacity: A Digital Audio Editor
*
* M3TopAppBar
*
* The Material 3 top app bar in the small, centre aligned, medium and large
* sizes. With a scroll behavior attached it collapses from the large or medium
* size down to the small size as its flickable scrolls, and its container
* colour lifts to surface container as soon as content sits underneath.
*
* API:
*     title, size ("small" | "centerAligned" | "medium" | "large"),
*     navigationIcon, actions (slot), flickable, navigationIconTriggered()
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Item {
    id: root

    property string title: ""
    property string size: "small"

    property int navigationIcon: IconCode.NONE
    property alias actions: actionsRow.data

    // Attach the scrolled content to collapse the bar and raise its colour.
    property Flickable flickable: null

    property NavigationPanel navigationPanel: null

    signal navigationIconTriggered()

    readonly property bool centered: root.size === "centerAligned"
    readonly property bool twoRow: root.size === "medium" || root.size === "large"

    readonly property real collapsedHeight: 64
    readonly property real expandedHeight: root.size === "large" ? 152
                                                                 : (root.size === "medium" ? 112 : 64)

    // 0 when fully expanded, 1 when fully collapsed.
    readonly property real collapseFraction: {
        if (!root.twoRow || !root.flickable) {
            return 0
        }
        var range = root.expandedHeight - root.collapsedHeight
        if (range <= 0) {
            return 0
        }
        return Math.max(0, Math.min(1, root.flickable.contentY / range))
    }

    readonly property bool scrolled: root.flickable ? root.flickable.contentY > 0 : false

    implicitHeight: root.expandedHeight - (root.expandedHeight - root.collapsedHeight) * root.collapseFraction
    implicitWidth: 600

    Rectangle {
        id: background

        anchors.fill: parent
        color: root.scrolled ? M3.color.surfaceContainer : M3.color.surface

        Behavior on color {
            ColorAnimation {
                duration: M3.motion.short4
                easing: M3.motion.standard
            }
        }
    }

    M3IconButton {
        id: navigationButton

        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.top: parent.top
        anchors.topMargin: (root.collapsedHeight - height) / 2

        visible: root.navigationIcon !== IconCode.NONE
        icon: root.navigationIcon
        accessibleName: "Navigation"
        navigation.panel: root.navigationPanel
        navigation.column: 0

        onClicked: root.navigationIconTriggered()
    }

    Row {
        id: actionsRow

        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.top: parent.top
        anchors.topMargin: (root.collapsedHeight - height) / 2
        spacing: 0
    }

    StyledTextLabel {
        id: titleLabel

        text: root.title
        color: M3.color.onSurface
        elide: Text.ElideRight

        // The two row sizes drop the headline to a second line and shrink it
        // back into the top row as the content scrolls.
        font: root.twoRow && root.collapseFraction < 0.5
              ? (root.size === "large" ? M3.typography.headlineMedium : M3.typography.headlineSmall)
              : M3.typography.titleLarge

        horizontalAlignment: root.centered ? Text.AlignHCenter : Text.AlignLeft

        /*
         * The centre aligned size reserves the same width on both sides, which
         * keeps the headline on the centre line of the bar without needing a
         * horizontalCenter anchor next to the left and right anchors.
         */
        readonly property real sideReserve: Math.max(navigationButton.visible ? navigationButton.width : 0,
                                                     actionsRow.width)

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: root.centered
                            ? titleLabel.sideReserve + 16
                            : (navigationButton.visible ? navigationButton.width + 8 : 16)
        anchors.rightMargin: root.centered ? titleLabel.sideReserve + 16 : actionsRow.width + 16

        y: root.twoRow
           ? (root.collapsedHeight - titleLabel.height) / 2
             + (1.0 - root.collapseFraction) * (root.expandedHeight - root.collapsedHeight - 8)
           : (root.collapsedHeight - titleLabel.height) / 2
    }

    M3Divider {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        visible: root.scrolled
    }
}
