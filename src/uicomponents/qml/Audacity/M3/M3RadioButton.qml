/*
* Audacity: A Digital Audio Editor
*
* M3RadioButton
*
* The Material 3 radio button. Use several of them inside one NavigationPanel
* so that the arrow keys move between the choices.
*
* Replaces: Muse.UiComponents RoundedRadioButton.
*
* API:
*     checked, text, enabled, toggled(), navigation
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property bool checked: false
    property string text: ""
    property string accessibleName: root.text

    property alias navigation: navCtrl

    signal toggled

    implicitHeight: Math.max(40, label.implicitHeight)
    implicitWidth: 40 + (label.visible ? label.implicitWidth + 4 : 0)

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3RadioButton"
        enabled: root.enabled && root.visible
        accessible.role: MUAccessible.RadioButton
        accessible.name: root.accessibleName
        accessible.checked: root.checked

        onActiveChanged: {
            if (navCtrl.active && !root.activeFocus) {
                root.forceActiveFocus()
            }
        }

        onTriggered: root.toggled()
    }

    Item {
        id: circleArea

        width: 40
        height: 40
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter

        M3StateLayer {
            anchors.fill: parent
            radius: 20
            color: root.checked ? M3.color.primary : M3.color.onSurface
            active: root.enabled
            hovered: mouseArea.containsMouse
            pressed: mouseArea.containsPress
            focused: navCtrl.highlight
        }

        Rectangle {
            id: outerCircle

            anchors.centerIn: parent
            width: 20
            height: 20
            radius: 10
            color: "transparent"
            antialiasing: true

            border.width: 2
            border.color: {
                if (!root.enabled) {
                    return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
                }
                return root.checked ? M3.color.primary : M3.color.onSurfaceVariant
            }

            Rectangle {
                anchors.centerIn: parent
                width: root.checked ? 10 : 0
                height: width
                radius: width / 2
                antialiasing: true
                color: root.enabled ? M3.color.primary : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)

                Behavior on width {
                    NumberAnimation {
                        duration: M3.motion.short3
                        easing: M3.motion.emphasizedDecelerate
                    }
                }
            }
        }

        M3FocusRing {
            anchors.fill: outerCircle
            shapeRadius: outerCircle.radius
            visible: navCtrl.highlight
        }
    }

    StyledTextLabel {
        id: label

        anchors.left: circleArea.right
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

        onClicked: {
            navCtrl.requestActive()
            root.toggled()
        }
    }
}
