/*
* Audacity: A Digital Audio Editor
*
* M3Button
*
* The Material 3 common button in all five variants: filled, tonal, outlined,
* text and elevated. Carries an optional leading icon, an optional loading
* state, a state layer, a ripple, a focus ring and muse navigation.
*
* Replaces: Muse.UiComponents FlatButton.
*
* API:
*     text, icon, variant, enabled, loading, accentButton, minWidth
*     navigation (NavigationControl), clicked()
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property string text: ""
    property int icon: IconCode.NONE

    // One of "filled", "tonal", "outlined", "text" or "elevated".
    property string variant: "filled"

    // Shows a spinner in place of the label and blocks activation.
    property bool loading: false

    property string toolTipTitle: ""
    property string toolTipDescription: ""
    property string toolTipShortcut: ""

    property string accessibleName: root.text

    property real minWidth: 48
    property real horizontalPadding: root.icon !== IconCode.NONE ? 16 : 24

    property alias navigation: navCtrl
    property alias mouseArea: mouseArea

    signal clicked

    readonly property bool interactive: root.enabled && !root.loading

    implicitHeight: M3.density.apply(40)
    implicitWidth: Math.max(root.minWidth, contentRow.implicitWidth + root.horizontalPadding * 2)

    readonly property color containerColor: {
        if (!root.enabled) {
            return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContainer)
        }
        switch (root.variant) {
        case "tonal":
            return M3.color.secondaryContainer
        case "elevated":
            return M3.surfaceAt(1)
        case "outlined":
        case "text":
            return "transparent"
        default:
            return M3.color.primary
        }
    }

    readonly property color contentColor: {
        if (!root.enabled) {
            return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
        }
        switch (root.variant) {
        case "tonal":
            return M3.color.onSecondaryContainer
        case "outlined":
        case "text":
        case "elevated":
            return M3.color.primary
        default:
            return M3.color.onPrimary
        }
    }

    readonly property int elevationLevel: {
        if (root.variant !== "elevated" || !root.enabled) {
            return 0
        }
        return mouseArea.containsPress ? 1 : (mouseArea.containsMouse ? 2 : 1)
    }

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3Button"
        enabled: root.interactive && root.visible
        accessible.role: MUAccessible.Button
        accessible.name: root.accessibleName
        accessible.enabled: root.interactive

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
        radius: M3.density.apply(40) / 2
        color: root.containerColor
        border.width: root.variant === "outlined" ? 1 : 0
        border.color: root.enabled ? M3.color.outline : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContainer)
        antialiasing: true

        Behavior on color {
            ColorAnimation {
                duration: M3.motion.short3
                easing: M3.motion.standard
            }
        }

        M3Elevation {
            anchors.fill: parent
            level: root.elevationLevel
            radius: background.radius
        }

        M3StateLayer {
            anchors.fill: parent
            radius: background.radius
            color: root.contentColor
            active: root.interactive
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
        visible: !root.loading

        StyledIconLabel {
            anchors.verticalCenter: parent.verticalCenter
            iconCode: root.icon
            visible: root.icon !== IconCode.NONE
            color: root.contentColor
        }

        StyledTextLabel {
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            visible: root.text !== ""
            font: M3.typography.labelLarge
            color: root.contentColor
        }
    }

    M3CircularProgress {
        anchors.centerIn: parent
        visible: root.loading
        running: root.loading
        indicatorColor: root.contentColor
        implicitSize: 20
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.interactive
        cursorShape: Qt.PointingHandCursor

        onPressed: function (mouse) {
            ripple.press(Qt.point(mouse.x, mouse.y))
        }

        onClicked: {
            navCtrl.requestActive()
            root.clicked()
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
