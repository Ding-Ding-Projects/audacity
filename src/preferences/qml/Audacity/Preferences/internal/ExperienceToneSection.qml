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

    title: qsTrc("preferences", "Tone")

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodyMedium
        text: qsTrc("preferences", "These sliders change tone only; facts stay unchanged.")
    }

    Column {
        width: parent.width
        spacing: 8

        StyledTextLabel {
            horizontalAlignment: Text.AlignLeft
            text: qsTrc("preferences", "English funny level")
        }

        M3Slider {
            id: englishSlider

            width: parent.width
            from: 1
            to: 5
            stepSize: 1
            value: root.settingsModel ? root.settingsModel.englishFunnyLevel : 5
            valueText: Math.round(englishSlider.value).toString()
            accessibleName: qsTrc("preferences", "English funny level, 1 is plain and 5 is most playful")

            navigation.panel: root.navigation
            navigation.name: "EnglishFunnyLevel"
            navigation.row: 1

            onMoved: root.settingsModel.englishFunnyLevel = Math.round(englishSlider.value)
        }
    }

    Column {
        width: parent.width
        spacing: 8

        StyledTextLabel {
            horizontalAlignment: Text.AlignLeft
            text: qsTrc("preferences", "Cantonese funny level")
        }

        M3Slider {
            id: cantoneseSlider

            width: parent.width
            from: 1
            to: 5
            stepSize: 1
            value: root.settingsModel ? root.settingsModel.cantoneseFunnyLevel : 5
            valueText: Math.round(cantoneseSlider.value).toString()
            accessibleName: qsTrc("preferences", "Cantonese funny level, 1 is plain and 5 is most playful")

            navigation.panel: root.navigation
            navigation.name: "CantoneseFunnyLevel"
            navigation.row: 2

            onMoved: root.settingsModel.cantoneseFunnyLevel = Math.round(cantoneseSlider.value)
        }
    }

    M3Switch {
        id: emojiSwitch

        text: qsTrc("preferences", "Emoji in dialogs and notifications")
        accessibleName: qsTrc("preferences", "Emoji in dialogs and notifications")
        checked: root.settingsModel ? root.settingsModel.emojiInDialogs : true

        navigation.panel: root.navigation
        navigation.name: "EmojiSwitch"
        navigation.row: 3

        onToggled: function (checked) {
            root.settingsModel.emojiInDialogs = checked
        }
    }

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodySmall
        text: qsTrc("preferences", "Emoji appear in message bodies only, never in buttons, labels or names read out by a screen reader.")
    }

    M3Card {
        width: parent.width
        variant: "outlined"
        padding: 12
        implicitHeight: previewLabel.implicitHeight + 24

        StyledTextLabel {
            id: previewLabel

            width: parent.width - 24
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            font: M3.typography.bodyMedium
            // The preview is a function call, so the levels are named here to
            // make the binding follow them.
            text: {
                if (!root.settingsModel) {
                    return ""
                }
                var follows = root.settingsModel.englishFunnyLevel + root.settingsModel.cantoneseFunnyLevel + root.settingsModel.languageMode + (root.settingsModel.emojiInDialogs ? 1 : 0)
                return follows >= 0 ? root.settingsModel.previewMessage(0, qsTrc("preferences", "3 tracks were exported to project.wav.")) : ""
            }
        }
    }

    M3Button {
        text: qsTrc("preferences", "Show an example notification")
        variant: "outlined"

        navigation.panel: root.navigation
        navigation.name: "ExampleNotification"
        navigation.row: 4

        onClicked: root.settingsModel.showExampleNotification()
    }
}
