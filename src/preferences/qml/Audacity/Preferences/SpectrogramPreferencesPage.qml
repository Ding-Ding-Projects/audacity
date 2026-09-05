/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.UiComponents

import Audacity.AppShell
import Audacity.Spectrogram
import Audacity.M3

PreferencesPage {
    id: root

    property bool showSelectionSectionTitle: selectionSection.showTitle
    property AbstractSpectrogramSettingsModel settingsModel: GlobalSpectrogramSettingsModel {}

    height: mainColumn.height

    Column {
        id: mainColumn
        width: parent.width
        spacing: 16

        SpectrogramSelectionSection {
            id: selectionSection

            settingsModel: root.settingsModel

            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart + 1

            onActiveFocusRequested: function (rect) {
                root.ensureContentVisibleRequested(rect)
            }
        }

        M3Divider {}

        SpectrogramScaleSection {
            settingsModel: root.settingsModel

            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart + 2

            onActiveFocusRequested: function (rect) {
                root.ensureContentVisibleRequested(rect)
            }
        }

        M3Divider {}

        SpectrogramColorsSection {
            settingsModel: root.settingsModel

            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart + 3

            onActiveFocusRequested: function (rect) {
                root.ensureContentVisibleRequested(rect)
            }
        }

        M3Divider {}

        SpectrogramAlgorithmSection {
            settingsModel: root.settingsModel

            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart + 4

            onActiveFocusRequested: function (rect) {
                root.ensureContentVisibleRequested(rect)
            }
        }
    }
}
