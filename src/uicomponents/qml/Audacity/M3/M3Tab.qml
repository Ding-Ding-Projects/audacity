/*
* Audacity: A Digital Audio Editor
*
* M3Tab
*
* One tab inside M3Tabs. Normally created by M3Tabs from its model rather than
* used directly.
*
* Replaces: Muse.UiComponents StyledTabButton.
*
* API:
*     text, icon, selected, primary, orientation, badgeCount, clicked()
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
    property bool selected: false

    // A primary tab uses a short indicator under the label and the primary
    // colour. A secondary tab uses a full width indicator and on surface.
    property bool primary: true

    property int orientation: Qt.Horizontal
    property int badgeCount: 0

    property string accessibleName: root.text

    property alias navigation: navCtrl

    signal clicked()

    readonly property bool vertical: root.orientation === Qt.Vertical

    readonly property color contentColor: {
        if (!root.enabled) {
            return Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g,
                           M3.color.onSurface.b, M3.stateLayer.disabledContent)
        }
        if (!root.selected) {
            return M3.color.onSurfaceVariant
        }
        return root.primary ? M3.color.primary : M3.color.onSurface
    }

    implicitHeight: root.vertical ? M3.density.apply(48) : M3.density.apply(48)
    implicitWidth: root.vertical ? 200 : Math.max(90, contentRow.implicitWidth + 32)

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3Tab"
        enabled: root.enabled && root.visible
        accessible.role: MUAccessible.ListItem
        accessible.name: root.accessibleName
        accessible.selected: root.selected
        accessible.visualItem: stateLayer

        onActiveChanged: {
            if (navCtrl.active && !root.activeFocus) {
                root.forceActiveFocus()
            }
        }

        onTriggered: root.clicked()
    }

    M3StateLayer {
        id: stateLayer

        anchors.fill: parent
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

    Row {
        id: contentRow

        anchors.centerIn: parent
        spacing: 8

        StyledIconLabel {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.icon !== IconCode.NONE
            iconCode: root.icon
            color: root.contentColor
        }

        StyledTextLabel {
            anchors.verticalCenter: parent.verticalCenter
            visible: root.text !== ""
            text: root.text
            font: M3.typography.titleSmall
            color: root.contentColor
        }

        M3Badge {
            anchors.verticalCenter: parent.verticalCenter
            count: root.badgeCount
        }
    }

    // Selection indicator.
    Rectangle {
        id: indicator

        visible: root.selected
        color: root.primary ? M3.color.primary : M3.color.onSurface
        antialiasing: true

        width: root.vertical ? 3 : (root.primary ? Math.min(root.width - 32, contentRow.width + 16) : root.width)
        height: root.vertical ? root.height : 3
        radius: root.vertical ? 1.5 : 1.5

        x: root.vertical ? 0 : (root.width - indicator.width) / 2
        y: root.vertical ? 0 : root.height - indicator.height

        Behavior on width {
            NumberAnimation {
                duration: M3.motion.short4
                easing: M3.motion.emphasized
            }
        }
    }

    M3FocusRing {
        anchors.fill: parent
        shapeRadius: 0
        visible: navCtrl.highlight
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
