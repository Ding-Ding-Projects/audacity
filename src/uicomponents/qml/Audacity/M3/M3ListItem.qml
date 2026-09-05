/*
* Audacity: A Digital Audio Editor
*
* M3ListItem
*
* A Material 3 list item with one, two or three lines. It takes a leading icon
* or arbitrary leading content, a headline, a supporting line, an overline and
* trailing content such as text, a switch or a checkbox.
*
* Replaces: Muse.UiComponents ListItemBlank.
*
* API:
*     headline, supportingText, overline, leadingIcon, trailingText,
*     selected, clickable, clicked(), navigation, leading and trailing slots
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property string headline: ""
    property string supportingText: ""
    property string overline: ""

    property int leadingIcon: IconCode.NONE
    property string trailingText: ""

    property bool selected: false
    property bool clickable: true

    property string accessibleName: root.headline

    // Slots for arbitrary content on either end.
    property Component leadingContent: null
    property Component trailingContent: null

    property alias navigation: navCtrl

    signal clicked

    readonly property int lineCount: {
        var lines = 1
        if (root.supportingText !== "") {
            lines += 1
        }
        if (root.overline !== "") {
            lines += 1
        }
        return lines
    }

    implicitHeight: M3.density.apply(root.lineCount === 1 ? 56 : (root.lineCount === 2 ? 72 : 88))
    implicitWidth: 320

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3ListItem"
        enabled: root.clickable && root.enabled && root.visible
        accessible.role: MUAccessible.ListItem
        accessible.name: root.accessibleName
        accessible.selected: root.selected
        accessible.visualItem: background

        onActiveChanged: {
            if (navCtrl.active && !root.activeFocus) {
                root.forceActiveFocus()
            }
        }

        onTriggered: root.clicked()
    }

    Rectangle {
        id: background

        anchors.fill: parent
        color: root.selected ? M3.color.secondaryContainer : "transparent"

        Behavior on color {
            ColorAnimation {
                duration: M3.motion.short3
                easing: M3.motion.standard
            }
        }

        M3StateLayer {
            anchors.fill: parent
            color: root.selected ? M3.color.onSecondaryContainer : M3.color.onSurface
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
        shapeRadius: 0
        visible: navCtrl.highlight
    }

    Item {
        id: leadingSlot

        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        width: visible ? Math.max(24, leadingLoader.width) : 0
        height: parent.height
        visible: root.leadingIcon !== IconCode.NONE || root.leadingContent !== null

        StyledIconLabel {
            anchors.centerIn: parent
            visible: root.leadingIcon !== IconCode.NONE
            iconCode: root.leadingIcon
            color: M3.color.onSurfaceVariant
        }

        Loader {
            id: leadingLoader

            anchors.centerIn: parent
            sourceComponent: root.leadingContent
        }
    }

    Column {
        anchors.left: leadingSlot.visible ? leadingSlot.right : parent.left
        anchors.leftMargin: 16
        anchors.right: trailingSlot.left
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 2

        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            visible: root.overline !== ""
            text: root.overline
            font: M3.typography.labelSmall
            color: M3.color.onSurfaceVariant
        }

        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            text: root.headline
            font: M3.typography.bodyLarge
            color: root.selected ? M3.color.onSecondaryContainer : M3.color.onSurface
        }

        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            visible: root.supportingText !== ""
            text: root.supportingText
            font: M3.typography.bodyMedium
            color: M3.color.onSurfaceVariant
            maximumLineCount: 2
            wrapMode: Text.WordWrap
            elide: Text.ElideRight
        }
    }

    Item {
        id: trailingSlot

        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        width: Math.max(trailingLabel.visible ? trailingLabel.width : 0, trailingLoader.item ? trailingLoader.width : 0)
        height: parent.height

        StyledTextLabel {
            id: trailingLabel

            anchors.centerIn: parent
            visible: root.trailingText !== ""
            text: root.trailingText
            font: M3.typography.labelSmall
            color: M3.color.onSurfaceVariant
        }

        Loader {
            id: trailingLoader

            anchors.centerIn: parent
            sourceComponent: root.trailingContent
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: root.clickable
        enabled: root.clickable && root.enabled
        z: -1

        onPressed: function (mouse) {
            ripple.press(Qt.point(mouse.x, mouse.y))
        }

        onClicked: {
            navCtrl.requestActive()
            root.clicked()
        }
    }
}
