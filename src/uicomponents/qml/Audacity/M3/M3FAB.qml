/*
* Audacity: A Digital Audio Editor
*
* M3FAB
*
* The Material 3 floating action button in the small, regular, large and
* extended sizes. The extended size shows a label next to the icon.
*
* Replaces: Muse.UiComponents FlatButton used as a primary accent action.
*
* API:
*     icon, text, size ("small" | "regular" | "large" | "extended"),
*     variant ("primary" | "secondary" | "tertiary" | "surface"),
*     lowered, navigation, clicked()
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property int icon: IconCode.NONE
    property string text: ""

    property string size: "regular"
    property string variant: "primary"

    // A lowered floating action button rests at elevation 1 instead of 3.
    property bool lowered: false

    property string accessibleName: root.text
    property string toolTipTitle: ""
    property string toolTipDescription: ""

    property alias navigation: navCtrl

    signal clicked()

    readonly property bool extended: root.size === "extended"

    readonly property real diameter: {
        switch (root.size) {
        case "small": return 40
        case "large": return 96
        default: return 56
        }
    }

    readonly property real cornerRadius: {
        switch (root.size) {
        case "small": return M3.shape.medium
        case "large": return M3.shape.extraLarge
        default: return M3.shape.large
        }
    }

    implicitHeight: root.extended ? 56 : root.diameter
    implicitWidth: root.extended
                   ? Math.max(80, contentRow.implicitWidth + 32)
                   : root.diameter

    readonly property color containerColor: {
        switch (root.variant) {
        case "secondary": return M3.color.secondaryContainer
        case "tertiary": return M3.color.tertiaryContainer
        case "surface": return M3.surfaceAt(3)
        default: return M3.color.primaryContainer
        }
    }

    readonly property color contentColor: {
        switch (root.variant) {
        case "secondary": return M3.color.onSecondaryContainer
        case "tertiary": return M3.color.onTertiaryContainer
        case "surface": return M3.color.primary
        default: return M3.color.onPrimaryContainer
        }
    }

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3FAB"
        enabled: root.enabled && root.visible
        accessible.role: MUAccessible.Button
        accessible.name: root.accessibleName !== "" ? root.accessibleName : root.toolTipTitle

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
        radius: root.extended ? M3.shape.large : root.cornerRadius
        color: root.containerColor
        antialiasing: true

        M3Elevation {
            anchors.fill: parent
            level: root.lowered ? (mouseArea.containsMouse ? 2 : 1)
                                : (mouseArea.containsMouse ? 4 : 3)
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
        spacing: root.extended ? 12 : 0

        StyledIconLabel {
            anchors.verticalCenter: parent.verticalCenter
            iconCode: root.icon
            color: root.contentColor
            font.pixelSize: root.size === "large" ? 36 : 24
        }

        StyledTextLabel {
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            visible: root.extended && root.text !== ""
            font: M3.typography.labelLarge
            color: root.contentColor
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor

        onPressed: function(mouse) {
            ripple.press(Qt.point(mouse.x, mouse.y))
        }

        onClicked: {
            navCtrl.requestActive()
            root.clicked()
        }
    }
}
