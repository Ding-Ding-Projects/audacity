/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.UiComponents
import Audacity.M3

BaseSection {
    id: root

    property var settingsModel: null

    // A confirmed active School mode removes the entire route. A record that
    // cannot be read stays visible so the unavailable state is never hidden.
    visible: !settingsModel || !settingsModel.schoolModeOn || !settingsModel.schoolModeAvailable

    title: qsTrc("preferences", "Language mode")

    M3SegmentedButton {
        id: modeSelector

        width: parent.width

        model: [
            {
                text: qsTrc("preferences", "English")
            },
            {
                text: qsTrc("preferences", "Cantonese (Hong Kong)")
            },
            {
                text: qsTrc("preferences", "Bilingual")
            }
        ]

        currentIndex: root.settingsModel ? root.settingsModel.languageMode : 0
        // School mode removes this route entirely. An unreadable shared
        // record is different: keep the current choice visible but prevent a
        // change while the runtime uses its conservative English fallback.
        visible: !root.settingsModel || !root.settingsModel.schoolModeOn
        enabled: !root.settingsModel || root.settingsModel.schoolModeAvailable

        navigationPanel: root.navigation
        navigationRowStart: 1

        onActivated: function (index) {
            root.settingsModel.languageMode = index
        }
    }

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodyMedium
        visible: !root.settingsModel || !root.settingsModel.schoolModeOn
        text: qsTrc("preferences", "Bilingual shows the English text and the Cantonese text together, separated by a slash.")
    }

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.error
        font: M3.typography.bodyMedium
        visible: root.settingsModel && !root.settingsModel.schoolModeAvailable
        text: root.settingsModel ? root.settingsModel.schoolModeError : ""
    }

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.error
        font: M3.typography.bodyMedium
        visible: root.settingsModel && !root.settingsModel.cantoneseCatalogueAvailable
        text: qsTrc("preferences", "The Cantonese (Hong Kong) translation could not be found, so bilingual mode shows English only.")
    }

    Row {
        spacing: 12
        visible: root.settingsModel && root.settingsModel.restartRequired

        StyledIconLabel {
            iconCode: IconCode.INFO
            color: M3.color.secondary
        }

        StyledTextLabel {
            width: root.width - 40
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            font: M3.typography.bodyMedium
            text: qsTrc("preferences", "Most of the interface changes straight away. Text that the operating system draws, and text that was already placed on screen, follows after a restart.")
        }
    }
}
