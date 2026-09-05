/*
* Audacity: A Digital Audio Editor
*
* M3MenuItem
*
* One row inside an M3Menu. Shows a leading icon or selection tick, a label, a
* right aligned shortcut and a trailing chevron when it opens a submenu.
*
* Replaces: Muse.UiComponents StyledMenuItem.
*
* API:
*     text, icon, shortcut, checkable, checked, hasSubMenu, isSeparator,
*     triggered(), navigation
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
    property string shortcut: ""

    property bool checkable: false
    property bool checked: false
    property bool hasSubMenu: false
    property bool isSeparator: false

    property string accessibleName: root.text

    property alias navigation: navCtrl
    property alias hovered: mouseArea.containsMouse

    signal triggered
    signal subMenuRequested

    implicitHeight: root.isSeparator ? 9 : M3.density.apply(48)
    implicitWidth: Math.max(112, contentRow.implicitWidth + 24)

    readonly property color contentColor: root.enabled ? M3.color.onSurface : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)

    M3Divider {
        visible: root.isSeparator
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.right: parent.right
        inset: 12
    }

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3MenuItem"
        enabled: root.enabled && root.visible && !root.isSeparator
        accessible.role: MUAccessible.MenuItem
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

    function activate() {
        if (root.hasSubMenu) {
            root.subMenuRequested()
            return
        }
        root.triggered()
    }

    Rectangle {
        id: background

        anchors.fill: parent
        anchors.margins: 4
        visible: !root.isSeparator
        radius: M3.shape.extraSmall
        antialiasing: true
        color: "transparent"

        M3StateLayer {
            anchors.fill: parent
            radius: background.radius
            color: M3.color.onSurface
            active: root.enabled
            hovered: mouseArea.containsMouse
            pressed: mouseArea.containsPress
            focused: navCtrl.highlight
        }
    }

    M3FocusRing {
        anchors.fill: background
        shapeRadius: background.radius
        visible: navCtrl.highlight
    }

    Row {
        id: contentRow

        visible: !root.isSeparator
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: 12

        StyledIconLabel {
            anchors.verticalCenter: parent.verticalCenter
            width: 24
            iconCode: root.checkable && root.checked ? IconCode.TICK_RIGHT_ANGLE : root.icon
            visible: root.checkable || root.icon !== IconCode.NONE
            color: root.contentColor
        }

        StyledTextLabel {
            id: label

            anchors.verticalCenter: parent.verticalCenter
            width: contentRow.width - contentRow.spacing * 2 - (shortcutLabel.visible ? shortcutLabel.width + contentRow.spacing : 0) - (chevron.visible ? chevron.width + contentRow.spacing : 0) - 24
            horizontalAlignment: Text.AlignLeft
            text: root.text
            font: M3.typography.labelLarge
            color: root.contentColor
            elide: Text.ElideRight
        }

        StyledTextLabel {
            id: shortcutLabel

            anchors.verticalCenter: parent.verticalCenter
            visible: root.shortcut !== "" && !root.hasSubMenu
            text: root.shortcut
            font: M3.typography.labelLarge
            color: M3.color.onSurfaceVariant
        }

        StyledIconLabel {
            id: chevron

            anchors.verticalCenter: parent.verticalCenter
            visible: root.hasSubMenu
            iconCode: IconCode.SMALL_ARROW_RIGHT
            color: M3.color.onSurfaceVariant
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled && !root.isSeparator

        onContainsMouseChanged: {
            if (mouseArea.containsMouse && root.hasSubMenu) {
                root.subMenuRequested()
            }
        }

        onClicked: {
            navCtrl.requestActive()
            root.activate()
        }
    }
}
