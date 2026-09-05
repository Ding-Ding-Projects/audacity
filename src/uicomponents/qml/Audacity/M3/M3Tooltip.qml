/*
* Audacity: A Digital Audio Editor
*
* M3Tooltip
*
* A Material 3 plain or rich tooltip. A plain tooltip is a small inverse
* surface with one line of text. A rich tooltip is a level 2 container with a
* subhead, supporting text and up to two text actions.
*
* Replaces: Muse.UiComponents StyledToolTip for surfaces that need a rich
* tooltip. Prefer ui.tooltip.show for ordinary one line hints.
*
* API:
*     text, subhead, supportingText, rich, actionText, secondaryActionText,
*     show(), hide(), actionTriggered(), secondaryActionTriggered()
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Item {
    id: root

    property string text: ""
    property string subhead: ""
    property string supportingText: ""

    property bool rich: root.supportingText !== ""

    property string actionText: ""
    property string secondaryActionText: ""

    signal actionTriggered
    signal secondaryActionTriggered

    visible: false
    implicitWidth: container.implicitWidth
    implicitHeight: container.implicitHeight

    function show() {
        root.visible = true
    }

    function hide() {
        root.visible = false
    }

    Rectangle {
        id: container

        anchors.fill: parent
        radius: root.rich ? M3.shape.medium : M3.shape.extraSmall
        antialiasing: true
        color: root.rich ? M3.surfaceAt(2) : M3.color.inverseSurface
        border.width: root.rich ? 1 : 0
        border.color: M3.color.outlineVariant

        implicitWidth: Math.min(312, column.implicitWidth + (root.rich ? 32 : 16))
        implicitHeight: column.implicitHeight + (root.rich ? 24 : 8)

        M3Elevation {
            anchors.fill: parent
            level: root.rich ? 2 : 0
            radius: container.radius
        }

        Column {
            id: column

            anchors.centerIn: parent
            width: parent.width - (root.rich ? 32 : 16)
            spacing: 4

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                visible: root.rich && root.subhead !== ""
                text: root.subhead
                font: M3.typography.titleSmall
                color: M3.color.onSurfaceVariant
            }

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                text: root.rich ? root.supportingText : root.text
                font: root.rich ? M3.typography.bodyMedium : M3.typography.bodySmall
                color: root.rich ? M3.color.onSurfaceVariant : M3.color.inverseOnSurface
            }

            Row {
                visible: root.rich && (root.actionText !== "" || root.secondaryActionText !== "")
                spacing: 8

                M3Button {
                    visible: root.actionText !== ""
                    variant: "text"
                    text: root.actionText
                    onClicked: root.actionTriggered()
                }

                M3Button {
                    visible: root.secondaryActionText !== ""
                    variant: "text"
                    text: root.secondaryActionText
                    onClicked: root.secondaryActionTriggered()
                }
            }
        }
    }
}
