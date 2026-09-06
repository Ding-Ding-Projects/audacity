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

    property string credentialText: ""

    title: qsTrc("preferences", "School mode")

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodyMedium
        text: qsTrc("preferences", "One shared mode for every app on this computer. While it is on, everything switches to English, and Cantonese, bilingual presentation, the funny level sliders, personal vocabulary and the dim sum surprise all behave as if they were not installed. This is a self imposed speed bump, not a security feature.")
    }

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurface
        font: M3.typography.bodyMedium
        text: root.settingsModel && root.settingsModel.schoolModeOn
              ? qsTrc("preferences", "%1 is on.").arg(root.settingsModel.schoolModeDisplayName)
              : qsTrc("preferences", "%1 is off.").arg(root.settingsModel ? root.settingsModel.schoolModeDisplayName : qsTrc("preferences", "School mode"))
    }

    M3TextField {
        id: renameField

        width: Math.min(parent.width, 360)
        label: qsTrc("preferences", "Rename this mode")
        currentText: root.settingsModel ? root.settingsModel.schoolModeDisplayName : ""

        onTextEditingFinished: function (text) {
            if (root.settingsModel && text.length > 0) {
                root.settingsModel.renameSchoolMode(text)
            }
        }
    }

    M3TextField {
        id: credentialField

        width: Math.min(parent.width, 360)
        isPassword: true
        label: root.settingsModel && root.settingsModel.schoolModeHasCredential
               ? qsTrc("preferences", "Enter the unlock PIN or password")
               : qsTrc("preferences", "Set an unlock PIN or password")
        supportingText: qsTrc("preferences", "Needed to turn this back off. If you forget it, delete the shared record file to reset it.")
        currentText: root.credentialText

        onTextEdited: function (text) {
            root.credentialText = text
        }
    }

    M3Button {
        text: root.settingsModel && root.settingsModel.schoolModeOn
              ? qsTrc("preferences", "Turn off")
              : qsTrc("preferences", "Turn on")

        onClicked: {
            if (!root.settingsModel) {
                return
            }
            const ok = root.settingsModel.schoolModeOn
                ? root.settingsModel.turnSchoolModeOff(root.credentialText)
                : root.settingsModel.turnSchoolModeOn(root.credentialText)
            if (ok) {
                root.credentialText = ""
                credentialField.currentText = ""
            }
        }
    }
}
