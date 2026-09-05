/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.UiComponents

import Audacity.UiComponents
import Audacity.M3

BaseSection {
    id: root

    // No translation needed
    title: "ASIO"
    spacing: 16

    property var apiModel: null

    Row {
        width: parent.width
        spacing: root.spacing

        M3Switch {
            id: m3Switch3
            width: root.columnWidth
            anchors.verticalCenter: parent.verticalCenter

            text: qsTrc("preferences", "Use device sample rate")

            checked: apiModel.asioUseDeviceSampleRate

            navigation.name: "AsioUseDeviceSampleRateBox"
            navigation.panel: root.navigation
            navigation.row: 1
            navigation.column: 0

            onToggled: {
                apiModel.setAsioUseDeviceSampleRate(checked)
                m3Switch3.checked = Qt.binding(function () {
                    return apiModel.asioUseDeviceSampleRate
                })
            }
        }

        M3Button {
            variant: "tonal"
            text: qsTrc("preferences", "Driver settings")

            navigation.name: "AsioDriverSettingsButton"
            navigation.panel: root.navigation
            navigation.row: 1
            navigation.column: 1

            onClicked: {
                apiModel.showAsioControlPanel()
            }
        }
    }
}
