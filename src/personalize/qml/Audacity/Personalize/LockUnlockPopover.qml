/*
* Audacity: A Digital Audio Editor
*
* LockUnlockPopover
*
* Shown when a locked element is activated. Presents whichever of PIN,
* password and one time code that element's lock policy needs, sharing one
* validator and one attempt budget across the keypad and the manual field.
*
* API:
*     openAt(anchorItem, elementId, elementLabel)
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Personalize

M3Dialog {
    id: root

    property string elementId: ""
    property string elementLabel: ""
    property string errorText: ""

    headline: qsTrc("personalize", "Unlock")

    function openAt(anchorItem, id, label) {
        root.elementId = id
        root.elementLabel = label
        root.errorText = ""
        pinField.currentText = ""
        passwordField.currentText = ""
        totpField.currentText = ""
        open()
    }

    function attempt() {
        var ok = LockRegistry.tryUnlock(root.elementId, pinField.currentText, passwordField.currentText, totpField.currentText)
        if (ok) {
            root.errorText = ""
            root.close()
        } else {
            root.errorText = qsTrc("personalize", "That did not match. Recovery is deleting the application's data folder, " + "shown on the Personalize preferences page.")
        }
    }

    Column {
        width: parent ? parent.width : 320
        spacing: 12

        StyledTextLabel {
            text: root.elementLabel
            font: M3.typography.titleSmall
        }

        M3TextField {
            id: pinField
            width: parent.width
            isPassword: true
            label: qsTrc("personalize", "PIN (leave blank if not required)")
        }

        // A simple access control style keypad sharing the same field.
        Grid {
            columns: 3
            spacing: 4
            Repeater {
                model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "", "0", "back"]
                delegate: M3Button {
                    required property string modelData
                    text: modelData === "back" ? qsTrc("personalize", "Del") : modelData
                    variant: "outlined"
                    visible: modelData !== ""
                    onClicked: {
                        if (modelData === "back") {
                            pinField.currentText = pinField.currentText.slice(0, -1)
                        } else {
                            pinField.currentText = pinField.currentText + modelData
                        }
                    }
                }
            }
        }

        M3TextField {
            id: passwordField
            width: parent.width
            isPassword: true
            label: qsTrc("personalize", "Password (leave blank if not required)")
        }
        M3TextField {
            id: totpField
            width: parent.width
            label: qsTrc("personalize", "One time code (leave blank if not required)")
            maximumLength: 8
        }

        StyledTextLabel {
            visible: root.errorText !== ""
            text: root.errorText
            wrapMode: Text.WordWrap
            width: parent.width
            color: M3.color.error
        }
    }

    actions: [
        M3Button {
            text: qsTrc("personalize", "Cancel")
            variant: "text"
            onClicked: root.close()
        },
        M3Button {
            text: qsTrc("personalize", "Unlock")
            variant: "filled"
            onClicked: root.attempt()
        }
    ]
}
