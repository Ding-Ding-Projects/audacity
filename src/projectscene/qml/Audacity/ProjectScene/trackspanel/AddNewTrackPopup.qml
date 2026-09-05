import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.ProjectScene
import Audacity.M3

StyledPopupView {
    id: root
    objectName: "AddNewTrackPanel"

    contentWidth: trackTypeOpts.width
    contentHeight: trackTypeOpts.height

    cornerRadius: M3.shape.large
    elevationLevel: 2
    backgroundColor: M3.surfaceAt(2)
    borderColor: M3.color.outlineVariant

    property alias popupAnchorItem: root.anchorItem

    signal createTrack(type: int)

    NavigationPanel {
        id: navPanel
        name: "AddNewTrackPopup"
        enabled: root.isOpened
        direction: NavigationPanel.Horizontal
        section: root.navigationSection
    }

    onOpened: {
        Qt.callLater(function () {
            root.repositionWindowIfNeed()
        })
        navPanel.requestActive()
    }

    RowLayout {
        id: trackTypeOpts

        spacing: 10

        Repeater {
            model: [
                {
                    type: TrackType.MONO,
                    icon: IconCode.CIRCLE,
                    text: qsTrc("projectscene", "Mono"),
                    enabled: true
                },
                {
                    type: TrackType.STEREO,
                    icon: IconCode.TWO_CIRCLES,
                    text: qsTrc("projectscene", "Stereo"),
                    enabled: true
                },
                {
                    type: TrackType.LABEL,
                    icon: IconCode.LOOP_IN,
                    text: qsTrc("projectscene", "Label"),
                    enabled: true
                }
            ]

            M3Card {
                Layout.preferredWidth: 80
                Layout.preferredHeight: 72
                Layout.fillHeight: true
                Layout.margins: 2

                variant: "outlined"
                clickable: true

                navigation.name: "TrackType" + index
                navigation.panel: navPanel
                navigation.column: index
                navigation.accessible.name: modelData.text

                enabled: modelData.enabled

                Column {
                    anchors.centerIn: parent
                    spacing: 8

                    StyledIconLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        iconCode: modelData.icon
                        color: M3.color.onSurface
                    }

                    StyledTextLabel {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: modelData.text
                        font: M3.typography.labelLarge
                        color: M3.color.onSurface
                    }
                }

                onClicked: {
                    createTrack(modelData.type)
                    root.close()
                }
            }
        }
    }
}
