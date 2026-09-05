/*
* Audacity: A Digital Audio Editor
*
* M3IconButton
*
* The Material 3 icon button in the standard, filled, tonal and outlined
* variants. It can also act as a toggle, in which case the selected state
* changes the container and content colours.
*
* Replaces: Muse.UiComponents FlatButton used with an icon and no text.
*
* API:
*     icon, variant, enabled, checkable, checked, toggled(), clicked()
*     navigation (NavigationControl), accessibleName
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property int icon: IconCode.NONE

    // One of "standard", "filled", "tonal" or "outlined".
    property string variant: "standard"

    property bool checkable: false
    property bool checked: false

    property string accessibleName: ""
    property string toolTipTitle: ""
    property string toolTipDescription: ""
    property string toolTipShortcut: ""

    property alias navigation: navCtrl
    property alias mouseArea: mouseArea

    signal clicked
    signal toggled(bool checked)

    implicitWidth: M3.density.apply(40)
    implicitHeight: M3.density.apply(40)

    readonly property color containerColor: {
        if (!root.enabled) {
            return root.variant === "standard" || root.variant === "outlined" ? "transparent" : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContainer)
        }
        switch (root.variant) {
        case "filled":
            return root.checkable && !root.checked ? M3.color.surfaceContainerHighest : M3.color.primary
        case "tonal":
            return root.checkable && !root.checked ? M3.color.surfaceContainerHighest : M3.color.secondaryContainer
        case "outlined":
            return root.checkable && root.checked ? M3.color.inverseSurface : "transparent"
        default:
            return "transparent"
        }
    }

    readonly property color contentColor: {
        if (!root.enabled) {
            return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
        }
        switch (root.variant) {
        case "filled":
            return root.checkable && !root.checked ? M3.color.primary : M3.color.onPrimary
        case "tonal":
            return root.checkable && !root.checked ? M3.color.onSurfaceVariant : M3.color.onSecondaryContainer
        case "outlined":
            return root.checkable && root.checked ? M3.color.inverseOnSurface : M3.color.onSurfaceVariant
        default:
            return root.checkable && root.checked ? M3.color.primary : M3.color.onSurfaceVariant
        }
    }

    function activate() {
        if (root.checkable) {
            root.checked = !root.checked
            root.toggled(root.checked)
        }
        root.clicked()
    }

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3IconButton"
        enabled: root.enabled && root.visible
        accessible.role: root.checkable ? MUAccessible.CheckBox : MUAccessible.Button
        accessible.name: root.accessibleName !== "" ? root.accessibleName : root.toolTipTitle
        accessible.checked: root.checked

        onActiveChanged: {
            if (navCtrl.active && !root.activeFocus) {
                root.forceActiveFocus()
            }
        }

        onTriggered: {
            ripple.pulse()
            root.activate()
        }
    }

    Rectangle {
        id: background

        anchors.fill: parent
        radius: width / 2
        color: root.containerColor
        border.width: root.variant === "outlined" && !root.checked ? 1 : 0
        border.color: M3.color.outline
        antialiasing: true

        Behavior on color {
            ColorAnimation {
                duration: M3.motion.short3
                easing: M3.motion.standard
            }
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

    StyledIconLabel {
        anchors.centerIn: parent
        iconCode: root.icon
        color: root.contentColor
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor

        onPressed: function (mouse) {
            ripple.press(Qt.point(mouse.x, mouse.y))
        }

        onClicked: {
            navCtrl.requestActive()
            root.activate()
        }
    }

    M3ToolTipHandler {
        target: root
        title: root.toolTipTitle
        description: root.toolTipDescription
        shortcut: root.toolTipShortcut
        hovered: mouseArea.containsMouse
    }
}
