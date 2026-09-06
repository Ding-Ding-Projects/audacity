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

    objectName: "ExperienceNarratorSection"

    property var settingsModel: null

    title: qsTrc("preferences", "Narrator")

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodyMedium
        text: qsTrc("preferences", "Speaks a short line for selected events. Off until you turn it on.")
    }

    M3Switch {
        text: qsTrc("preferences", "Narrator")
        accessibleName: qsTrc("preferences", "Narrator. Speaks selected events out loud")
        checked: root.settingsModel ? root.settingsModel.narratorEnabled : false

        navigation.panel: root.navigation
        navigation.name: "NarratorEnabled"
        navigation.row: 1

        onToggled: function (checked) {
            root.settingsModel.narratorEnabled = checked
        }
    }

    M3SegmentedButton {
        width: Math.min(parent.width, 420)
        model: [qsTrc("preferences", "English"), qsTrc("preferences", "Cantonese"), qsTrc("preferences", "Both")]
        currentIndex: root.settingsModel ? root.settingsModel.narratorLanguage : 0

        onActivated: function (index) {
            root.settingsModel.narratorLanguage = index
        }
    }

    M3Switch {
        text: qsTrc("preferences", "Quiet mode")
        accessibleName: qsTrc("preferences", "Quiet mode. Keeps the narrator completely silent")
        checked: root.settingsModel ? root.settingsModel.narratorQuietMode : false

        navigation.panel: root.navigation
        navigation.name: "NarratorQuietMode"
        navigation.row: 2

        onToggled: function (checked) {
            root.settingsModel.narratorQuietMode = checked
        }
    }

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodySmall
        text: qsTrc("preferences", "Quiet mode keeps the narrator silent even while it is otherwise on. The narrator also ducks under an active screen reader on its own.")
    }

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodySmall
        text: root.settingsModel ? root.settingsModel.narratorEngineDescription : ""
    }

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodySmall
        text: qsTrc("preferences", "Choose automatically is the default for each language. Voice pickers list whatever the active speech engine reports on this machine.")
    }
}
