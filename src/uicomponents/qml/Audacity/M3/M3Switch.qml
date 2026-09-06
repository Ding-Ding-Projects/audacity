/*
* Audacity: A Digital Audio Editor
*
* M3Switch
*
* The Material 3 switch. The handle grows while pressed and can carry a tick
* or a cross icon, which is the accessible way to tell the two states apart
* without relying on colour alone.
*
* Replaces: Muse.UiComponents ToggleButton.
*
* API:
*     checked, enabled, showIcon, text, toggled(checked), navigation
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property bool checked: false

    // Draws a tick when on and a cross when off.
    property bool showIcon: true

    property string text: ""
    property string accessibleName: root.text

    property alias navigation: navCtrl

    // Personalize appearance override hookup, see M3Button.qml for detail.
    property string elementId: ""
    property int appearanceRevision: 0

    function m3Appearance(property, fallback) {
        root.appearanceRevision
        if (root.elementId === "" || typeof AppearanceOverrides === "undefined") {
            return fallback
        }
        return AppearanceOverrides.resolve(root.elementId, "", property, fallback)
    }

    Connections {
        target: typeof AppearanceOverrides !== "undefined" ? AppearanceOverrides : null
        ignoreUnknownSignals: true

        function onElementChanged(elementId) {
            if (elementId === root.elementId) {
                root.appearanceRevision = root.appearanceRevision + 1
            }
        }
    }

    signal toggled(bool checked)

    implicitWidth: track.width + (label.visible ? label.implicitWidth + 12 : 0)
    implicitHeight: 32

    function activate() {
        root.checked = !root.checked
        root.toggled(root.checked)
    }

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3Switch"
        enabled: root.enabled && root.visible
        accessible.role: MUAccessible.CheckBox
        accessible.name: root.accessibleName
        accessible.checked: root.checked

        onActiveChanged: {
            if (navCtrl.active && !root.activeFocus) {
                root.forceActiveFocus()
            }
        }

        onTriggered: root.activate()
    }

    Rectangle {
        id: track

        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter

        width: 52
        height: 32
        radius: root.m3Appearance("radius", 16)
        antialiasing: true

        readonly property color defaultColor: {
            if (!root.enabled) {
                return root.checked ? Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, 0.12) : Qt.rgba(M3.color.surfaceContainerHighest.r, M3.color.surfaceContainerHighest.g, M3.color.surfaceContainerHighest.b, 0.12)
            }
            return root.checked ? M3.color.primary : M3.color.surfaceContainerHighest
        }

        color: root.m3Appearance("containerColor", track.defaultColor)

        border.width: root.checked ? 0 : 2
        border.color: root.enabled ? M3.color.outline : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContainer)

        Behavior on color {
            ColorAnimation {
                duration: M3.motion.short4
                easing: M3.motion.emphasized
            }
        }

        Rectangle {
            id: handle

            readonly property real restingSize: root.checked ? 24 : (root.showIcon ? 24 : 16)

            width: mouseArea.containsPress ? 28 : handle.restingSize
            height: handle.width
            radius: handle.width / 2
            antialiasing: true

            anchors.verticalCenter: parent.verticalCenter
            x: root.checked ? track.width - handle.width - 4 : 4

            color: {
                if (!root.enabled) {
                    return root.checked ? M3.color.surface : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
                }
                return root.checked ? M3.color.onPrimary : M3.color.outline
            }

            Behavior on x {
                NumberAnimation {
                    duration: M3.motion.short4
                    easing: M3.motion.emphasized
                }
            }

            Behavior on width {
                NumberAnimation {
                    duration: M3.motion.short2
                    easing: M3.motion.standard
                }
            }

            Behavior on color {
                ColorAnimation {
                    duration: M3.motion.short4
                    easing: M3.motion.emphasized
                }
            }

            StyledIconLabel {
                anchors.centerIn: parent
                visible: root.showIcon
                iconCode: root.checked ? IconCode.TICK_RIGHT_ANGLE : IconCode.CLOSE_X_ROUNDED
                font.pixelSize: 16
                color: root.checked ? M3.color.onPrimaryContainer : M3.color.surfaceContainerHighest
            }
        }

        M3StateLayer {
            width: 40
            height: 40
            radius: 20
            anchors.verticalCenter: handle.verticalCenter
            anchors.horizontalCenter: handle.horizontalCenter
            color: root.checked ? M3.color.primary : M3.color.onSurface
            active: root.enabled
            hovered: mouseArea.containsMouse
            pressed: mouseArea.containsPress
            focused: navCtrl.highlight
        }
    }

    M3FocusRing {
        anchors.fill: track
        shapeRadius: track.radius
        visible: navCtrl.highlight
    }

    StyledTextLabel {
        id: label

        anchors.left: track.right
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter

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
            root.activate()
        }
    }
}
