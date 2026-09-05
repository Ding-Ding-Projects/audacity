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

    readonly property bool hasFile: root.settingsModel && root.settingsModel.vocabularyFileName !== ""
    readonly property bool hasError: root.settingsModel && root.settingsModel.vocabularyError !== ""

    title: qsTrc("preferences", "Personal vocabulary")

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodyMedium
        text: qsTrc("preferences", "Replace words in the interface with your own. The file stays on this computer and its contents are never written to the log.")
    }

    Row {
        spacing: 12

        M3Button {
            text: qsTrc("preferences", "Choose JSON file")
            variant: root.hasFile ? "outlined" : "filled"

            navigation.panel: root.navigation
            navigation.name: "ChooseVocabularyFile"
            navigation.row: 1

            onClicked: root.settingsModel.chooseVocabularyFile()
        }

        M3Button {
            text: qsTrc("preferences", "Clear")
            variant: "text"
            visible: root.hasFile

            navigation.panel: root.navigation
            navigation.name: "ClearVocabulary"
            navigation.row: 2

            onClicked: root.settingsModel.clearVocabulary()
        }
    }

    StyledTextLabel {
        id: statusLabel

        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        font: M3.typography.bodyMedium
        color: root.hasError ? M3.color.error : M3.color.onSurfaceVariant

        text: {
            if (!root.settingsModel) {
                return ""
            }
            if (root.hasError) {
                return qsTrc("preferences", "That file could not be used: %1").arg(root.settingsModel.vocabularyError)
            }
            if (root.hasFile) {
                return qsTrc("preferences", "Loaded: %1 terms from %2").arg(root.settingsModel.vocabularyEntryCount).arg(root.settingsModel.vocabularyFileName)
            }
            return qsTrc("preferences", "No file chosen.")
        }
    }

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodySmall
        text: qsTrc("preferences", "Choosing another file replaces the one in use. Up to 2000 terms and 256 KB.")
    }
}
