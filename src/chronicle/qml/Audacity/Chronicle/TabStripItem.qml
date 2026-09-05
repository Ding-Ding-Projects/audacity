/*
* Audacity: A Digital Audio Editor
*
* TabStripItem
*
* One tab in a TabStrip. It carries the group colour as a leading bar, a pin
* marker, the label and a close button. The label always reads left to right,
* whichever side the strip is docked to.
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Item {
    id: root

    property string tabId: ""
    property string text: ""
    property int icon: IconCode.NONE
    property bool selected: false
    property bool pinned: false
    property bool closable: true
    property bool iconsOnly: false
    property color groupColor: "transparent"

    property alias navigation: navCtrl

    signal clicked
    signal closeRequested
    signal contextMenuRequested
    signal moveRequested(int delta)

    implicitWidth: root.iconsOnly ? M3.density.apply(48) : Math.min(M3.density.apply(220), M3.density.apply(96) + label.implicitWidth)
    implicitHeight: M3.density.apply(40)

    readonly property color contentColor: root.selected ? M3.color.onSecondaryContainer : M3.color.onSurfaceVariant

    NavigationControl {
        id: navCtrl

        name: "Tab." + root.tabId
        enabled: root.enabled && root.visible
        // The muse accessibility roles have no dedicated tab role. A tab is
        // reported as a list item inside the strip's list, together with its
        // selected state, which is what a screen reader announces.
        accessible.role: MUAccessible.ListItem
        accessible.name: root.text + (root.pinned ? " " + qsTrc("chronicle", "pinned") : "")
        accessible.selected: root.selected
        accessible.visualItem: background

        onTriggered: root.clicked()
    }

    Rectangle {
        id: background

        anchors.fill: parent
        radius: M3.shape.small
        antialiasing: true
        color: root.selected ? M3.color.secondaryContainer : "transparent"

        M3StateLayer {
            anchors.fill: parent
            radius: background.radius
            color: M3.color.onSurface
            hovered: mouseArea.containsMouse
            pressed: mouseArea.containsPress
            focused: navCtrl.highlight
        }
    }

    // The group colour, drawn as a bar rather than as the only signal, so the
    // grouping is not carried by colour alone: the group name is in the
    // accessible description and in the tab list.
    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: 2
        anchors.verticalCenter: parent.verticalCenter
        width: 3
        height: parent.height - 12
        radius: 2
        antialiasing: true
        visible: root.groupColor !== "transparent"
        color: root.groupColor
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.right: closeButton.left
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        StyledIconLabel {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.icon !== IconCode.NONE
            iconCode: root.icon
            color: root.contentColor
        }

        StyledIconLabel {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.pinned
            iconCode: IconCode.LOCK_CLOSED
            color: root.contentColor
        }

        StyledTextLabel {
            id: label

            anchors.verticalCenter: parent.verticalCenter
            visible: !root.iconsOnly
            width: Math.min(implicitWidth, parent.width - 24)
            horizontalAlignment: Text.AlignLeft
            elide: Text.ElideRight
            text: root.text
            font: M3.typography.labelLarge
            color: root.contentColor
        }
    }

    M3IconButton {
        id: closeButton

        anchors.right: parent.right
        anchors.rightMargin: 2
        anchors.verticalCenter: parent.verticalCenter
        visible: root.closable && !root.iconsOnly
        icon: IconCode.CLOSE_X_ROUNDED
        accessibleName: qsTrc("chronicle", "Close %1").arg(root.text)

        onClicked: root.closeRequested()
    }

    M3FocusRing {
        anchors.fill: background
        shapeRadius: background.radius
        visible: navCtrl.highlight
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor

        property real pressX: 0
        property real pressY: 0

        onPressed: function (mouse) {
            mouseArea.pressX = mouse.x
            mouseArea.pressY = mouse.y
        }

        onClicked: function (mouse) {
            navCtrl.requestActive()
            if (mouse.button === Qt.RightButton) {
                root.contextMenuRequested()
                return
            }
            root.clicked()
        }

        // Dragging a tab past its own extent moves it one place, which is the
        // reorder gesture without a drag layer that would fight the strip's
        // own scrolling.
        onPositionChanged: function (mouse) {
            if (!mouseArea.pressed) {
                return
            }
            var dx = mouse.x - mouseArea.pressX
            var dy = mouse.y - mouseArea.pressY
            if (dx > root.width * 0.6 || dy > root.height * 0.9) {
                mouseArea.pressX = mouse.x
                mouseArea.pressY = mouse.y
                root.moveRequested(1)
            } else if (dx < -root.width * 0.6 || dy < -root.height * 0.9) {
                mouseArea.pressX = mouse.x
                mouseArea.pressY = mouse.y
                root.moveRequested(-1)
            }
        }
    }

    Keys.onPressed: function (event) {
        if (event.modifiers & Qt.ShiftModifier) {
            if (event.key === Qt.Key_Right || event.key === Qt.Key_Down) {
                root.moveRequested(1)
                event.accepted = true
            } else if (event.key === Qt.Key_Left || event.key === Qt.Key_Up) {
                root.moveRequested(-1)
                event.accepted = true
            }
        }
    }
}
