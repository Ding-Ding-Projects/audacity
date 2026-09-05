/*
* Audacity: A Digital Audio Editor
*
* M3Card
*
* The Material 3 card in the elevated, filled and outlined variants. A card may
* be made clickable, in which case it gains a state layer, a ripple, a focus
* ring and a navigation control.
*
* API:
*     variant ("elevated" | "filled" | "outlined"), clickable, clicked(),
*     navigation, default content property
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    default property alias content: contentContainer.data

    property string variant: "elevated"
    property bool clickable: false
    property string accessibleName: ""

    property real padding: 16

    property alias navigation: navCtrl

    signal clicked

    implicitWidth: 280
    implicitHeight: contentContainer.childrenRect.height + root.padding * 2

    readonly property int restingElevation: root.variant === "elevated" ? 1 : 0

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3Card"
        enabled: root.clickable && root.enabled && root.visible
        accessible.role: MUAccessible.Button
        accessible.name: root.accessibleName
        accessible.visualItem: background

        onActiveChanged: {
            if (navCtrl.active && !root.activeFocus) {
                root.forceActiveFocus()
            }
        }

        onTriggered: {
            ripple.pulse()
            root.clicked()
        }
    }

    Rectangle {
        id: background

        anchors.fill: parent
        radius: M3.shape.medium
        antialiasing: true

        color: {
            switch (root.variant) {
            case "filled":
                return M3.color.surfaceContainerHighest
            case "outlined":
                return M3.color.surface
            default:
                return M3.surfaceAt(1)
            }
        }

        border.width: root.variant === "outlined" ? 1 : 0
        border.color: M3.color.outlineVariant

        M3Elevation {
            anchors.fill: parent
            level: root.clickable && mouseArea.containsMouse ? root.restingElevation + 1 : root.restingElevation
            radius: background.radius
        }

        M3StateLayer {
            anchors.fill: parent
            radius: background.radius
            color: M3.color.onSurface
            active: root.clickable && root.enabled
            hovered: mouseArea.containsMouse
            pressed: mouseArea.containsPress
            focused: navCtrl.highlight
        }

        M3Ripple {
            id: ripple

            anchors.fill: parent
            color: M3.color.onSurface
        }
    }

    M3FocusRing {
        anchors.fill: background
        shapeRadius: background.radius
        visible: navCtrl.highlight
    }

    Item {
        id: contentContainer

        anchors.fill: parent
        anchors.margins: root.padding
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: root.clickable
        enabled: root.clickable && root.enabled
        visible: root.clickable
        cursorShape: Qt.PointingHandCursor

        onPressed: function (mouse) {
            ripple.press(Qt.point(mouse.x, mouse.y))
        }

        onClicked: {
            navCtrl.requestActive()
            root.clicked()
        }
    }
}
