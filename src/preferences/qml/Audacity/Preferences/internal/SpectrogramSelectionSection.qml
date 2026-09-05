/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick 2.15
import QtQuick.Controls 2.15

import Muse.UiComponents

import Audacity.UiComponents 1.0
import Audacity.Spectrogram 1.0
import Audacity.M3

SpectrogramBaseSection {
    id: root

    property bool showTitle: true

    title: root.showTitle ? qsTrc("preferences", "Selection") : ""

    required property AbstractSpectrogramSettingsModel settingsModel

    M3Switch {
        id: m3Switch5
        text: qsTrc("preferences/spectrogram", "Enable spectral selection")
        checked: settingsModel.spectralSelectionEnabled

        onToggled: {
            settingsModel.spectralSelectionEnabled = !settingsModel.spectralSelectionEnabled;
            m3Switch5.checked = Qt.binding(function () {
                return settingsModel.spectralSelectionEnabled;
            });
        }

        navigation.panel: root.navigation
        navigation.name: "SpectralSelectionEnabledCheckBox"
    }
}
