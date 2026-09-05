/*
* Audacity: A Digital Audio Editor
*
* M3Checkbox
*
* The Material 3 checkbox with an unchecked, checked and indeterminate state.
* The 18 pixel box sits inside a 40 pixel state layer so that the touch target
* keeps the Material minimum.
*
* Replaces: Muse.UiComponents CheckBox.
*
* API:
*     checked, indeterminate, text, enabled, clicked(), navigation
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property bool checked: false
    property bool indeterminate: false

    property string text: ""
    property string accessibleName: root.text

    property alias navigation: navCtrl

    signal clicked

    readonly property bool marked: root.checked || root.indeterminate

    implicitHeight: Math.max(40, label.implicitHeight)
    implicitWidth: 40 + (label.visible ? label.implicitWidth + 4 : 0)

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3Checkbox"
        enabled: root.enabled && root.visible
        accessible.role: MUAccessible.CheckBox
        accessible.name: root.accessibleName
        accessible.checked: root.checked

        onActiveChanged: {
            if (navCtrl.active && !root.activeFocus) {
                root.forceActiveFocus()
            }
        }

        onTriggered: root.clicked()
    }

    Item {
        id: boxArea

        width: 40
        height: 40
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter

        M3StateLayer {
            anchors.fill: parent
            radius: 20
            color: root.marked ? M3.color.primary : M3.color.onSurface
            active: root.enabled
            hovered: mouseArea.containsMouse
            pressed: mouseArea.containsPress
            focused: navCtrl.highlight
        }

        M3Ripple {
            id: ripple

            anchors.fill: parent
            color: root.marked ? M3.color.primary : M3.color.onSurface
        }

        Rectangle {
            id: box

            anchors.centerIn: parent
            width: 18
            height: 18
            radius: 2
            antialiasing: true

            color: root.marked ? (root.enabled ? M3.color.primary : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContainer)) : "transparent"

            border.width: root.marked ? 0 : 2
            border.color: root.enabled ? M3.color.onSurfaceVariant : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)

            Behavior on color {
                ColorAnimation {
                    duration: M3.motion.short3
                    easing: M3.motion.standard
                }
            }

            StyledIconLabel {
                anchors.centerIn: parent
                visible: root.checked && !root.indeterminate
                iconCode: IconCode.TICK_RIGHT_ANGLE
                font.pixelSize: 14
                color: M3.color.onPrimary
            }

            Rectangle {
                anchors.centerIn: parent
                visible: root.indeterminate
                width: 10
                height: 2
                radius: 1
                color: M3.color.onPrimary
            }
        }

        M3FocusRing {
            anchors.fill: box
            shapeRadius: box.radius
            visible: navCtrl.highlight
        }
    }

    StyledTextLabel {
        id: label

        anchors.left: boxArea.right
        anchors.leftMargin: 4
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        horizontalAlignment: Text.AlignLeft
        text: root.text
        visible: root.text !== ""
        font: M3.typography.bodyMedium
        color: root.enabled ? M3.color.onSurface : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor

        onPressed: function (mouse) {
            ripple.press(Qt.point(mouse.x - boxArea.x, mouse.y - boxArea.y))
        }

        onClicked: {
            navCtrl.requestActive()
            root.clicked()
        }
    }
}
