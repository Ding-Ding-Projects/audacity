import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Muse.Ui
import Muse.UiComponents
import Muse.GraphicalEffects

import Audacity.M3

Rectangle {
    id: root

    property string iconUrl: ""
    property string title: ""
    property string subtitle: ""
    property string effectCode: ""

    property alias navigation: getItButton.navigation

    signal getEffectClicked(string code)

    QtObject {
        id: prv

        readonly property int spaceS: 4
        readonly property int spaceM: 8
        readonly property int spaceL: 12
        readonly property int spaceXL: 16
        readonly property int spaceXXL: 24

        readonly property int borderWidth: 1
        readonly property int borderRadius: 4

        readonly property int cardWidth: 304
        readonly property int cardHeight: 120
        readonly property int iconSize: 96
        readonly property int buzySize: 32
        readonly property int iconPlaceholderSize: 48
        readonly property int getItButtonWidth: 172
        readonly property int getItButtonHeight: 24
    }

    width: prv.cardWidth
    height: prv.cardHeight

    radius: prv.borderRadius
    color: M3.color.surface
    border.width: prv.borderWidth
    border.color: M3.color.outlineVariant

    RowLayout {
        anchors.fill: parent
        anchors.margins: prv.spaceL
        spacing: prv.spaceL

        Rectangle {
            id: previewRect

            Layout.preferredWidth: prv.iconSize
            Layout.preferredHeight: prv.iconSize
            radius: prv.borderRadius
            color: effectImage.status === Image.Ready ? "transparent" : M3.color.surfaceContainer
            border.width: effectImage.status === Image.Ready ? 0 : prv.borderWidth
            border.color: M3.color.outlineVariant
            clip: true

            Image {
                id: effectImage
                anchors.fill: parent
                source: root.iconUrl
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                visible: status === Image.Ready

                layer.enabled: ui.isEffectsAllowed
                layer.effect: RoundedCornersEffect {
                    radius: previewRect.radius
                }
            }

            StyledIconLabel {
                anchors.centerIn: parent
                iconCode: IconCode.PLUGIN
                font.pixelSize: prv.iconPlaceholderSize
                color: M3.color.onSurface
                visible: effectImage.status !== Image.Ready && effectImage.status !== Image.Loading
            }

            StyledBusyIndicator {
                anchors.centerIn: parent
                width: prv.buzySize
                height: prv.buzySize
                visible: effectImage.status === Image.Loading
                running: visible
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: prv.spaceS

            StyledTextLabel {
                Layout.fillWidth: true
                text: root.title
                font: M3.typography.titleMedium
                horizontalAlignment: Text.AlignLeft
                maximumLineCount: 2
                elide: Text.ElideRight
                wrapMode: Text.Wrap
            }

            StyledTextLabel {
                Layout.fillWidth: true
                text: root.subtitle
                font: M3.typography.bodyMedium
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.Wrap
                maximumLineCount: 2
                elide: Text.ElideRight
            }

            Item {
                Layout.fillHeight: true
            }

            M3Button {
                id: getItButton

                Layout.preferredHeight: prv.getItButtonHeight
                Layout.preferredWidth: prv.getItButtonWidth

                text: qsTrc("projectscene", "Get it on MuseHub")
                accentButton: true

                navigation.name: root.title
                navigation.accessible.name: root.title + ". " + root.subtitle + ". " + text

                onClicked: root.getEffectClicked(root.effectCode)
            }
        }
    }
}
