import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Item {
    id: root

    property alias navigation: buttonContainer.navigation
    property alias realtimeEffectsNavigation: effectsTitleBar.navigation
    property alias closeEffectsNavigation: effectsTitleBar.navigation
    property alias addTrackNavigation: addNewTrackBtn.navigation

    property int effectsSectionWidth: 0
    property bool showEffectsSection: false

    property int buttonWidth: 97
    property int buttonHeight: M3.density.apply(40)
    property int buttonRightMargin: 8
    property int textLeftMargin: 12

    signal effectsSectionCloseRequested
    signal addRequested(type: int)

    Component.onCompleted: {
        if (effectsSectionWidth == 0) {
            console.warn("effectsSectionWidth is not set ; doing some guesswork")
            effectsSectionWidth = 240
        }
    }

    onShowEffectsSectionChanged: {
        if (addNewTrack.isOpened) {
            addNewTrack.close()
        }
    }

    RowLayout {
        id: rowLayout

        anchors.fill: parent

        spacing: 0

        Rectangle {
            id: effectsTitleBar

            property int padding: parent.height / 4
            property NavigationPanel navigation: NavigationPanel {
                name: "RealtimeEffectsSectionPanel"
                enabled: root.enabled && root.visible && root.showEffectsSection
                section: buttonContainer.navigation.section
                direction: NavigationPanel.Vertical
                order: 0

                accessible.name: qsTrc("projectscene", "Real-time effects panel")
            }

            Layout.preferredWidth: root.effectsSectionWidth
            Layout.preferredHeight: root.height

            color: M3.color.surfaceContainer
            border.color: "transparent"
            border.width: padding
            visible: root.showEffectsSection

            StyledTextLabel {
                anchors.fill: parent
                padding: effectsTitleBar.padding

                text: qsTrc("projectscene", "Realtime effects")
                font: M3.typography.titleSmall
                color: M3.color.onSurface
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
            }

            MouseArea {
                anchors.fill: parent
            }

            Rectangle {
                anchors.right: parent.right
                width: root.height
                height: root.height

                color: effectsTitleBar.color

                M3IconButton {
                    id: closeEffectsSectionButton

                    anchors.centerIn: parent
                    width: parent.width - 2 * effectsTitleBar.padding
                    height: parent.height - 2 * effectsTitleBar.padding

                    navigation.name: "CloseEffectsSection"
                    navigation.panel: effectsTitleBar.navigation
                    navigation.order: 0

                    //: Tooltip of the button that closes the panel
                    toolTipTitle: qsTrc("projectscene", "Close real-time effects panel")
                    accessibleName: qsTrc("projectscene", "Close real-time effects panel")

                    icon: IconCode.CLOSE_X_ROUNDED

                    onClicked: {
                        root.effectsSectionCloseRequested()
                    }
                }
            }
        }

        SeparatorLine {}

        Rectangle {
            id: buttonContainer

            color: M3.color.surfaceContainer

            width: root.verticalPanelDefaultWidth
            Layout.fillWidth: true
            Layout.preferredHeight: root.height

            property NavigationPanel navigation: NavigationPanel {
                name: "AddTrackPanel"
                enabled: root.enabled && root.visible
                order: closeEffectsSectionButton.navigation.order + 1

                accessible.name: qsTrc("projectscene", "Add track")
            }

            StyledTextLabel {
                anchors.left: buttonContainer.left
                anchors.leftMargin: root.textLeftMargin
                anchors.verticalCenter: buttonContainer.verticalCenter

                text: qsTrc("projectscene", "Tracks")
                font: M3.typography.titleSmall
                color: M3.color.onSurface
            }

            M3Button {
                id: addNewTrackBtn

                width: root.buttonWidth
                height: root.buttonHeight

                anchors.right: buttonContainer.right
                anchors.rightMargin: root.buttonRightMargin
                anchors.verticalCenter: buttonContainer.verticalCenter

                navigation.name: "AddTrack"
                navigation.panel: buttonContainer.navigation
                navigation.order: 0

                variant: "tonal"

                text: qsTrc("projectscene", "Add track")
                accessibleName: qsTrc("projectscene", "Add track")

                enabled: true

                icon: IconCode.PLUS

                onClicked: {
                    if (addNewTrack.isOpened) {
                        addNewTrack.close()
                    } else {
                        addNewTrack.open()
                    }
                }

                AddNewTrackPopup {
                    id: addNewTrack

                    onCreateTrack: type => {
                        root.addRequested(type)
                    }
                }
            }
        }
    }
}
