/*
* Audacity: A Digital Audio Editor
*
* M3Chip
*
* The Material 3 chip in the assist, filter, input and suggestion variants.
* A filter chip shows a leading tick when selected, an input chip shows a
* trailing remove button.
*
* API:
*     text, variant, icon, checked, elevated, toggled(checked), clicked(),
*     removed(), navigation
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property string text: ""

    // One of "assist", "filter", "input" or "suggestion".
    property string variant: "assist"

    property int icon: IconCode.NONE
    property bool checked: false
    property bool elevated: false

    property string accessibleName: root.text
    property string toolTipTitle: ""

    property alias navigation: navCtrl

    signal clicked
    signal toggled(bool checked)
    signal removed

    readonly property bool selectable: root.variant === "filter"
    readonly property bool removable: root.variant === "input"
    readonly property bool selected: root.selectable && root.checked

    implicitHeight: M3.density.apply(32)
    implicitWidth: contentRow.implicitWidth + 24

    readonly property color containerColor: {
        if (!root.enabled) {
            return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContainer)
        }
        if (root.selected) {
            return M3.color.secondaryContainer
        }
        return root.elevated ? M3.surfaceAt(1) : "transparent"
    }

    readonly property color contentColor: {
        if (!root.enabled) {
            return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
        }
        return root.selected ? M3.color.onSecondaryContainer : M3.color.onSurfaceVariant
    }

    function activate() {
        if (root.selectable) {
            root.checked = !root.checked
            root.toggled(root.checked)
        }
        root.clicked()
    }

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3Chip"
        enabled: root.enabled && root.visible
        accessible.role: root.selectable ? MUAccessible.CheckBox : MUAccessible.Button
        accessible.name: root.accessibleName
        accessible.checked: root.checked
        accessible.visualItem: background

        onActiveChanged: {
            if (navCtrl.active && !root.activeFocus) {
                root.forceActiveFocus()
            }
        }

        onTriggered: root.activate()
    }

    Rectangle {
        id: background

        anchors.fill: parent
        radius: M3.shape.small
        antialiasing: true
        color: root.containerColor

        border.width: root.selected || root.elevated ? 0 : 1
        border.color: root.enabled ? M3.color.outlineVariant : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContainer)

        Behavior on color {
            ColorAnimation {
                duration: M3.motion.short3
                easing: M3.motion.standard
            }
        }

        M3Elevation {
            anchors.fill: parent
            level: root.elevated ? 1 : 0
            radius: background.radius
        }

        M3StateLayer {
            anchors.fill: parent
            radius: background.radius
            color: root.contentColor
            active: root.enabled
            hovered: mouseArea.containsMouse
            pressed: mouseArea.containsPress
            focused: navCtrl.highlight
        }

        M3Ripple {
            id: ripple

            anchors.fill: parent
            color: root.contentColor
        }
    }

    M3FocusRing {
        anchors.fill: background
        shapeRadius: background.radius
        visible: navCtrl.highlight
    }

    Row {
        id: contentRow

        anchors.centerIn: parent
        spacing: 8

        StyledIconLabel {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.selected || root.icon !== IconCode.NONE
            iconCode: root.selected ? IconCode.TICK_RIGHT_ANGLE : root.icon
            font.pixelSize: 18
            color: root.contentColor
        }

        StyledTextLabel {
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            font: M3.typography.labelLarge
            color: root.contentColor
        }

        StyledIconLabel {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.removable
            iconCode: IconCode.CLOSE_X_ROUNDED
            font.pixelSize: 18
            color: root.contentColor

            MouseArea {
                anchors.fill: parent
                anchors.margins: -4
                enabled: root.removable && root.enabled
                cursorShape: Qt.PointingHandCursor
                onClicked: root.removed()
            }
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor
        z: -1

        onPressed: function (mouse) {
            ripple.press(Qt.point(mouse.x, mouse.y))
        }

        onClicked: {
            navCtrl.requestActive()
            root.activate()
        }
    }
}
