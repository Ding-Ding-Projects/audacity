/*
* Audacity: A Digital Audio Editor
*
* M3Dialog
*
* A Material 3 basic or full screen dialog built on the muse StyledDialogView,
* so the application's interactive provider still opens it by URI and the
* navigation section behaves like every other dialog.
*
* Replaces: Muse.UiComponents StyledDialogView used directly.
*
* API:
*     icon, headline, supportingText, fullScreen, actions (default slot),
*     default content slot, accept(), reject()
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

StyledDialogView {
    id: root

    default property alias body: bodyContainer.data

    // Buttons for the action row along the bottom right.
    property alias actions: actionsRow.data

    property int icon: IconCode.NONE
    property string headline: ""
    property string supportingText: ""

    property bool fullScreen: false

    // Personalize appearance override hookup, see M3Button.qml for detail.
    property string elementId: ""
    property int appearanceRevision: 0

    function m3Appearance(property, fallback) {
        root.appearanceRevision
        if (root.elementId === "" || typeof AppearanceOverrides === "undefined") {
            return fallback
        }
        return AppearanceOverrides.resolve(root.elementId, "", property, fallback)
    }

    Connections {
        target: typeof AppearanceOverrides !== "undefined" ? AppearanceOverrides : null
        ignoreUnknownSignals: true

        function onElementChanged(elementId) {
            if (elementId === root.elementId) {
                root.appearanceRevision = root.appearanceRevision + 1
            }
        }
    }

    contentWidth: root.fullScreen ? 900 : 420
    contentHeight: root.fullScreen ? 640 : column.implicitHeight + 48

    margins: root.fullScreen ? 0 : 24
    cornerRadius: root.m3Appearance("radius", root.fullScreen ? 0 : M3.shape.extraLarge)

    Rectangle {
        anchors.fill: parent
        color: root.m3Appearance("containerColor", M3.surfaceAt(3))
        radius: root.cornerRadius
        antialiasing: true
    }

    Column {
        id: column

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: 16

        StyledIconLabel {
            anchors.horizontalCenter: parent.horizontalCenter
            visible: root.icon !== IconCode.NONE
            iconCode: root.icon
            font.pixelSize: 24
            color: M3.color.secondary
        }

        StyledTextLabel {
            width: parent.width
            visible: root.headline !== ""
            text: root.headline
            font: M3.typography.headlineSmall
            color: M3.color.onSurface
            horizontalAlignment: root.icon !== IconCode.NONE ? Text.AlignHCenter : Text.AlignLeft
            wrapMode: Text.WordWrap
        }

        StyledTextLabel {
            width: parent.width
            visible: root.supportingText !== ""
            text: root.supportingText
            font: M3.typography.bodyMedium
            color: M3.color.onSurfaceVariant
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
        }

        Item {
            id: bodyContainer

            width: parent.width
            height: childrenRect.height
        }

        Row {
            id: actionsRow

            anchors.right: parent.right
            spacing: 8
        }
    }
}
