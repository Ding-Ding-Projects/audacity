/*
* Audacity: A Digital Audio Editor
*
* M3BottomSheet
*
* A Material 3 modal bottom sheet. It slides up from the bottom edge over a
* scrim, carries a drag handle and closes on escape, on a scrim click or on a
* downward drag.
*
* API:
*     opened, headline, showDragHandle, open(), close(), default content slot
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Item {
    id: root

    default property alias content: contentContainer.data

    property bool opened: false
    property string headline: ""
    property bool showDragHandle: true

    property real sheetHeight: Math.min(root.height * 0.6, contentContainer.childrenRect.height + 96)

    property alias navigationSection: navSec

    signal closed

    anchors.fill: parent
    visible: root.opened || sheet.y < root.height

    function open() {
        root.opened = true
    }

    function close() {
        root.opened = false
        root.closed()
    }

    NavigationSection {
        id: navSec

        name: "M3BottomSheet"
        enabled: root.opened
        type: NavigationSection.Exclusive
        // A NavigationSection with no explicit order fails its own
        // componentComplete assertion (order() > -1) and is never
        // registered, which crashed the whole application later when the
        // enclosing dialog that had already created this bottom sheet was
        // torn down. Any positive value works; 1 matches the other
        // dialog-level exclusive sections in this codebase.
        order: 1

        onActiveChanged: {
            if (navSec.active) {
                root.forceActiveFocus()
            }
        }
    }

    Rectangle {
        id: scrim

        anchors.fill: parent
        color: M3.color.scrim
        opacity: root.opened ? 0.32 : 0.0

        Behavior on opacity {
            NumberAnimation {
                duration: M3.motion.medium2
                easing: M3.motion.emphasized
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: root.opened
            onClicked: root.close()
        }
    }

    M3Surface {
        id: sheet

        level: 1
        anchors.left: parent.left
        anchors.right: parent.right
        height: root.sheetHeight

        topLeftRadius: M3.shape.extraLarge
        topRightRadius: M3.shape.extraLarge

        y: root.opened ? root.height - sheet.height : root.height

        Behavior on y {
            NumberAnimation {
                duration: M3.motion.medium4
                easing: M3.motion.emphasizedDecelerate
            }
        }

        Rectangle {
            id: dragHandle

            visible: root.showDragHandle
            anchors.horizontalCenter: parent.horizontalCenter
            y: 16
            width: 32
            height: 4
            radius: 2
            color: M3.color.onSurfaceVariant
            opacity: 0.4
        }

        StyledTextLabel {
            id: headlineLabel

            visible: root.headline !== ""
            anchors.left: parent.left
            anchors.leftMargin: 24
            anchors.right: parent.right
            anchors.rightMargin: 24
            anchors.top: parent.top
            anchors.topMargin: root.showDragHandle ? 36 : 24
            horizontalAlignment: Text.AlignLeft
            text: root.headline
            font: M3.typography.titleLarge
            color: M3.color.onSurface
        }

        Item {
            id: contentContainer

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: root.headline !== "" ? headlineLabel.bottom : dragHandle.bottom
            anchors.topMargin: 16
            anchors.bottom: parent.bottom
            anchors.margins: 24
        }

        MouseArea {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 48
            cursorShape: Qt.SizeVerCursor

            property real pressY: 0

            onPressed: function (mouse) {
                pressY = mouse.y
            }

            onReleased: function (mouse) {
                if (mouse.y - pressY > 40) {
                    root.close()
                }
            }
        }
    }

    Keys.onEscapePressed: root.close()
}
