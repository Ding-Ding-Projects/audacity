/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

// One notification toast. Built on the Material 3 card anatomy: a container at
// elevation 3, a leading state icon, a title, a body and an optional action.
M3Card {
    id: root

    property int notificationType: 0
    property string title: ""
    property string body: ""
    property string actionText: ""
    property bool persistent: false
    property int autoDismissDelay: 6000

    signal actionTriggered
    signal dismissed

    readonly property color accentColor: {
        switch (root.notificationType) {
        case 1:
            return M3.color.tertiary
        case 2:
            return M3.color.secondary
        case 3:
            return M3.color.error
        default:
            return M3.color.primary
        }
    }

    readonly property int stateIcon: {
        switch (root.notificationType) {
        case 1:
            return IconCode.TICK_RIGHT_ANGLE
        case 2:
            return IconCode.WARNING
        case 3:
            return IconCode.CLOSE_X_ROUNDED
        default:
            return IconCode.INFO
        }
    }

    variant: "elevated"
    padding: 16
    width: 380
    implicitHeight: layout.implicitHeight + root.padding * 2

    // The whole toast is one accessible item, so a screen reader reads the
    // title and the body together and does not stop at the icon.
    accessibleName: root.title + ". " + root.body

    Timer {
        id: autoDismiss

        interval: root.autoDismissDelay
        running: !root.persistent && root.visible && !hoverArea.containsMouse
        onTriggered: root.dismissed()
    }

    MouseArea {
        id: hoverArea

        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }

    Column {
        id: layout

        width: parent.width
        spacing: 8

        Row {
            width: parent.width
            spacing: 12

            StyledIconLabel {
                id: stateIconLabel

                iconCode: root.stateIcon
                color: root.accentColor
                font.pixelSize: 20
            }

            Column {
                width: parent.width - stateIconLabel.width - 12
                spacing: 4

                StyledTextLabel {
                    width: parent.width
                    horizontalAlignment: Text.AlignLeft
                    text: root.title
                    font: M3.typography.titleSmall
                }

                StyledTextLabel {
                    width: parent.width
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.WordWrap
                    maximumLineCount: 4
                    text: root.body
                    font: M3.typography.bodyMedium
                    color: M3.color.onSurfaceVariant
                }
            }
        }

        Row {
            anchors.right: parent.right
            spacing: 8

            M3Button {
                text: root.actionText
                variant: "text"
                visible: root.actionText !== ""

                onClicked: root.actionTriggered()
            }

            M3Button {
                //: Dismisses one notification toast
                text: qsTrc("experience", "Dismiss")
                variant: "text"

                onClicked: root.dismissed()
            }
        }
    }
}
