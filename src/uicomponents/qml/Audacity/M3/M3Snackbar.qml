/*
* Audacity: A Digital Audio Editor
*
* M3Snackbar
*
* One Material 3 snackbar: a short message on an inverse surface with an
* optional single action and an optional close button. Normally created by
* M3SnackbarHost rather than used directly.
*
* API:
*     text, actionText, showClose, duration, actionTriggered(), dismissed()
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Rectangle {
    id: root

    property string text: ""
    property string actionText: ""
    property bool showClose: false

    // Milliseconds before the snackbar dismisses itself. Zero keeps it open.
    property int duration: 4000

    signal actionTriggered
    signal dismissed

    implicitHeight: Math.max(48, label.implicitHeight + 28)
    implicitWidth: Math.min(600, Math.max(344, contentRow.implicitWidth + 32))

    radius: M3.shape.extraSmall
    antialiasing: true
    color: M3.color.inverseSurface

    M3Elevation {
        anchors.fill: parent
        level: 3
        radius: root.radius
    }

    Row {
        id: contentRow

        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: root.actionText !== "" || root.showClose ? 8 : 16
        spacing: 8

        StyledTextLabel {
            id: label

            anchors.verticalCenter: parent.verticalCenter
            width: contentRow.width - (actionButton.visible ? actionButton.width + 8 : 0) - (closeButton.visible ? closeButton.width + 8 : 0)
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
            text: root.text
            font: M3.typography.bodyMedium
            color: M3.color.inverseOnSurface
        }

        M3Button {
            id: actionButton

            anchors.verticalCenter: parent.verticalCenter
            visible: root.actionText !== ""
            variant: "text"
            text: root.actionText

            onClicked: {
                root.actionTriggered()
                root.dismissed()
            }
        }

        M3IconButton {
            id: closeButton

            anchors.verticalCenter: parent.verticalCenter
            visible: root.showClose
            icon: IconCode.CLOSE_X_ROUNDED
            accessibleName: "Dismiss"
            onClicked: root.dismissed()
        }
    }

    Timer {
        running: root.visible && root.duration > 0
        interval: root.duration
        onTriggered: root.dismissed()
    }

    AccessibleItem {
        visualItem: root
        role: MUAccessible.Information
        name: root.text
        enabled: root.visible
    }
}
