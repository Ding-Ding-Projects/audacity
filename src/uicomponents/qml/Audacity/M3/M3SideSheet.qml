/*
* Audacity: A Digital Audio Editor
*
* M3SideSheet
*
* A Material 3 side sheet, docked or modal, anchored to the leading or trailing
* edge. A docked sheet leaves the rest of the window usable and casts no scrim.
*
* API:
*     opened, headline, modal, edge (Qt.LeftEdge | Qt.RightEdge), sheetWidth,
*     open(), close(), default content slot
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
    property bool modal: true
    property string headline: ""
    property int edge: Qt.RightEdge
    property real sheetWidth: 320

    signal closed

    anchors.fill: parent
    visible: root.opened || sheet.x !== root.closedX

    readonly property bool trailing: root.edge === Qt.RightEdge
    readonly property real openX: root.trailing ? root.width - root.sheetWidth : 0
    readonly property real closedX: root.trailing ? root.width : -root.sheetWidth

    function open() {
        root.opened = true
    }

    function close() {
        root.opened = false
        root.closed()
    }

    Rectangle {
        anchors.fill: parent
        visible: root.modal
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
            enabled: root.opened && root.modal
            onClicked: root.close()
        }
    }

    M3Surface {
        id: sheet

        level: root.modal ? 1 : 0
        shadowVisible: root.modal

        y: 0
        height: root.height
        width: root.sheetWidth
        x: root.opened ? root.openX : root.closedX

        topLeftRadius: root.trailing ? M3.shape.large : 0
        bottomLeftRadius: root.trailing ? M3.shape.large : 0
        topRightRadius: root.trailing ? 0 : M3.shape.large
        bottomRightRadius: root.trailing ? 0 : M3.shape.large

        Behavior on x {
            NumberAnimation {
                duration: M3.motion.medium4
                easing: M3.motion.emphasizedDecelerate
            }
        }

        Item {
            id: header

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: root.headline !== "" ? 56 : 0
            visible: root.headline !== ""

            StyledTextLabel {
                anchors.left: parent.left
                anchors.leftMargin: 24
                anchors.right: closeButton.left
                anchors.verticalCenter: parent.verticalCenter
                horizontalAlignment: Text.AlignLeft
                text: root.headline
                font: M3.typography.titleLarge
                color: M3.color.onSurface
            }

            M3IconButton {
                id: closeButton

                anchors.right: parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                icon: IconCode.CLOSE_X_ROUNDED
                accessibleName: "Close"
                onClicked: root.close()
            }
        }

        Item {
            id: contentContainer

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: header.bottom
            anchors.bottom: parent.bottom
            anchors.margins: 16
        }
    }

    Keys.onEscapePressed: root.close()
}
