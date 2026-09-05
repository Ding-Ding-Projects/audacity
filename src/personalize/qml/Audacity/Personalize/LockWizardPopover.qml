/*
* Audacity: A Digital Audio Editor
*
* LockWizardPopover
*
* Creates a toy lock on one element: chooses a policy, collects whichever
* credentials that policy needs, and says plainly that this is just for fun
* rather than a security boundary.
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

    // Matches au::personalize::LockPolicy in lockregistry.h.
    readonly property var policies: [
        {
            value: 0,
            label: qsTrc("personalize", "PIN")
        },
        {
            value: 1,
            label: qsTrc("personalize", "Password")
        },
        {
            value: 2,
            label: qsTrc("personalize", "PIN and password")
        },
        {
            value: 3,
            label: qsTrc("personalize", "Password and one time code")
        },
        {
            value: 4,
            label: qsTrc("personalize", "PIN and one time code")
        },
        {
            value: 5,
            label: qsTrc("personalize", "Password, PIN and one time code")
        }
    ]

    property int selectedPolicy: 0
    readonly property bool needsPin: selectedPolicy === 0 || selectedPolicy === 2 || selectedPolicy === 4 || selectedPolicy === 5
    readonly property bool needsPassword: selectedPolicy === 1 || selectedPolicy === 2 || selectedPolicy === 3 || selectedPolicy === 5
    readonly property bool needsTotp: selectedPolicy === 3 || selectedPolicy === 4 || selectedPolicy === 5

    headline: qsTrc("personalize", "Lock this element")

    function openAt(anchorItem, id, label) {
        root.elementId = id
        root.elementLabel = label
        pinField.currentText = ""
        passwordField.currentText = ""
        totpSecretField.currentText = ""
        durationField.currentText = "0"
        lockedOnLaunchSwitch.checked = false
        open()
    }

    Column {
        width: parent ? parent.width : 360
        spacing: 12

        StyledTextLabel {
            text: qsTrc("personalize", "This is just for fun, not security: anyone with access to this computer can undo it by " + "deleting the application's data folder. It never encrypts anything.")
            wrapMode: Text.WordWrap
            width: parent.width
            font: M3.typography.bodySmall
        }

        M3Dropdown {
            id: policyDropdown
            width: parent.width
            model: root.policies.map(function (p) {
                return p.label
            })
            currentIndex: root.selectedPolicy
            onActivated: function (index) {
                root.selectedPolicy = index
            }
        }

        M3TextField {
            id: pinField
            width: parent.width
            visible: root.needsPin
            isPassword: true
            label: qsTrc("personalize", "PIN")
            maximumLength: 12
        }
        M3TextField {
            id: passwordField
            width: parent.width
            visible: root.needsPassword
            isPassword: true
            label: qsTrc("personalize", "Password")
        }
        M3TextField {
            id: totpSecretField
            width: parent.width
            visible: root.needsTotp
            label: qsTrc("personalize", "One time code secret (base32)")
            supportingText: qsTrc("personalize", "Set this up from the authenticator page, then paste the secret here")
        }

        M3TextField {
            id: durationField
            width: parent.width
            label: qsTrc("personalize", "Unlock duration in minutes (0 means until the app closes)")
            currentText: "0"
        }
        M3Switch {
            id: lockedOnLaunchSwitch
            text: qsTrc("personalize", "Locked again every time the application starts")
        }
    }

    actions: [
        M3Button {
            text: qsTrc("personalize", "Cancel")
            variant: "text"
            onClicked: root.close()
        },
        M3Button {
            text: qsTrc("personalize", "Create lock")
            variant: "filled"
            onClicked: {
                LockRegistry.createLock(root.elementId, root.selectedPolicy, pinField.currentText, passwordField.currentText, totpSecretField.currentText, parseInt(durationField.currentText) || 0, lockedOnLaunchSwitch.checked)
                root.close()
            }
        }
    ]
}
