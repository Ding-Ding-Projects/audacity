/*
* Audacity: A Digital Audio Editor
*/
import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents
import Audacity.M3

StyledDialogView {
    id: root

    title: qsTrc("effects", "Save preset")

    contentWidth: 280
    contentHeight: 85

    margins: 16

    readonly property string trimmedName: input.currentText.trim()
    readonly property string invalidReason: {
        if (/[\/\\]/.test(trimmedName)) {
            //: %1 is a forward slash, %2 is a backslash; neither character is allowed in a preset name.
            return qsTrc("effects", "Preset name cannot contain %1 or %2").arg("/").arg("\\")
        }
        return ""
    }
    readonly property bool isValid: trimmedName.length > 0 && invalidReason === ""

    Component.onCompleted: {
        Qt.callLater(input.forceActiveFocus)
    }

    NavigationPanel {
        id: presetNameNavPanel
        name: "PresetNameInput"
        enabled: input.enabled && input.visible
        section: root.navigationSection
        order: 0
    }

    QtObject {
        id: prv

        function submit() {
            if (!root.isValid) {
                return
            }
            root.ret = {
                errcode: 0,
                value: root.trimmedName
            }
            root.hide()
        }
    }

    M3TextField {
        id: input

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        navigation.panel: presetNameNavPanel
        navigation.order: 1
        navigation.name: "Preset name"

        //: Placeholder text of the preset name input field
        placeholder: qsTrc("effects", "Preset name")

        onTextEdited: t => input.currentText = t
        onAccepted: prv.submit()
    }

    StyledTextLabel {
        id: errorLabel

        anchors.top: input.bottom
        anchors.topMargin: 4
        anchors.left: parent.left
        anchors.right: parent.right

        horizontalAlignment: Text.AlignLeft

        visible: root.invalidReason !== ""
        Accessible.ignored: !visible

        text: root.invalidReason
        color: ui.theme.extra["error_text_color"]
        font: Qt.font(Object.assign({}, M3.typography.bodyMedium, {
            pointSize: M3.typography.bodyMedium.pointSize - 1
        }))
    }

    ButtonBox {
        id: bbox

        width: parent.width
        anchors.bottom: parent.bottom

        navigationPanel.section: root.navigationSection
        navigationPanel.order: 2

        M3Button {
            variant: "text"

            text: qsTrc("global", "Cancel")
            buttonRole: ButtonBoxModel.RejectRole
            buttonId: ButtonBoxModel.Cancel
            minWidth: 80

            onClicked: {
                root.reject()
            }
        }

        M3Button {
            variant: "filled"

            text: qsTrc("global", "OK")
            buttonRole: ButtonBoxModel.AcceptRole
            buttonId: ButtonBoxModel.Ok
            accentButton: true
            minWidth: 80
            enabled: root.isValid

            onClicked: {
                prv.submit()
            }
        }
    }
}
