/*
* Audacity: A Digital Audio Editor
*
* AuthenticatorPage
*
* A local, offline TOTP authenticator. Every code is computed on this
* machine from the machine clock and the stored secret; nothing here makes a
* network request. Pairing shows an in-process rendered QR code plus the
* manual base32 secret beside it, and confirms the pairing before the entry
* is kept.
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Personalize

Flickable {
    id: root

    contentWidth: width
    contentHeight: content.height
    clip: true

    AuthenticatorModel {
        id: model
    }

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: model.tickNow()
    }

    property bool addingNew: false
    property string pendingSecret: ""
    property string pendingUri: ""

    function startAdding() {
        root.pendingSecret = model.newSecretBase32()
        issuerField.currentText = qsTrc("personalize", "Material Audacity")
        accountField.currentText = ""
        confirmField.currentText = ""
        root.pendingUri = model.otpauthUriFor(issuerField.currentText, accountField.currentText, root.pendingSecret, 6, 30, 0)
        root.addingNew = true
    }

    Column {
        id: content
        width: root.width
        spacing: 16
        padding: 24

        StyledTextLabel {
            text: qsTrc("personalize", "Authenticator")
            font: M3.typography.headlineSmall
        }

        StyledTextLabel {
            visible: model.clockLooksSkewed
            text: qsTrc("personalize", "This machine's clock looks unusual. One time codes depend on the clock; if they are " + "rejected everywhere, check the date and time first.")
            wrapMode: Text.WordWrap
            width: parent.width - 48
            color: M3.color.error
        }

        Repeater {
            model: model.entries
            delegate: Rectangle {
                required property var modelData
                width: content.width - 48
                radius: M3.shape.medium
                color: M3.color.surfaceContainer
                implicitHeight: entryColumn.implicitHeight + 24

                Column {
                    id: entryColumn
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 4

                    StyledTextLabel {
                        text: modelData.issuer !== "" ? (modelData.issuer + " — " + modelData.account) : modelData.account
                        font: M3.typography.titleSmall
                    }
                    StyledTextLabel {
                        text: modelData.currentCode
                        font: M3.typography.displaySmall
                    }
                    StyledTextLabel {
                        text: qsTrc("personalize", "%1 seconds left, next code %2").arg(model.secondsRemainingInPeriod).arg(modelData.nextCode)
                        font: M3.typography.bodySmall
                    }
                    M3Button {
                        text: qsTrc("personalize", "Remove")
                        variant: "text"
                        onClicked: model.removeEntry(modelData.id)
                    }
                }
            }
        }

        M3Button {
            text: qsTrc("personalize", "Add an entry")
            variant: "outlined"
            onClicked: root.startAdding()
        }

        Column {
            visible: root.addingNew
            width: parent.width - 48
            spacing: 12

            StyledTextLabel {
                text: qsTrc("personalize", "Scan this with your authenticator, or type the secret in by hand.")
                wrapMode: Text.WordWrap
                width: parent.width
            }

            QrCodeImage {
                width: 180
                height: 180
                text: root.pendingUri
            }

            TextEdit {
                width: parent.width
                text: root.pendingSecret
                readOnly: true
                selectByMouse: true
                wrapMode: Text.WrapAnywhere
                Accessible.name: qsTrc("personalize", "The manual pairing secret, selectable to copy")
            }

            M3TextField {
                id: issuerField
                width: parent.width
                label: qsTrc("personalize", "Issuer")
                onTextEditingFinished: function () {
                    root.pendingUri = model.otpauthUriFor(issuerField.currentText, accountField.currentText, root.pendingSecret, 6, 30, 0)
                }
            }
            M3TextField {
                id: accountField
                width: parent.width
                label: qsTrc("personalize", "Account")
                onTextEditingFinished: function () {
                    root.pendingUri = model.otpauthUriFor(issuerField.currentText, accountField.currentText, root.pendingSecret, 6, 30, 0)
                }
            }
            M3TextField {
                id: confirmField
                width: parent.width
                label: qsTrc("personalize", "Type the current code to confirm pairing")
                maximumLength: 8
            }

            Row {
                spacing: 8
                M3Button {
                    text: qsTrc("personalize", "Cancel")
                    variant: "text"
                    onClicked: root.addingNew = false
                }
                M3Button {
                    text: qsTrc("personalize", "Confirm and add")
                    variant: "filled"
                    onClicked: {
                        var id = model.addManual(issuerField.currentText, accountField.currentText, root.pendingSecret, 6, 30, 0)
                        if (id !== "" && model.currentCode(id) === confirmField.currentText) {
                            root.addingNew = false
                        } else if (id !== "") {
                            model.removeEntry(id)
                        }
                    }
                }
            }
        }

        M3TextField {
            id: importField
            width: parent.width - 48
            label: qsTrc("personalize", "Or paste an otpauth:// link")
        }
        M3Button {
            text: qsTrc("personalize", "Import")
            variant: "outlined"
            onClicked: model.addFromOtpauthUri(importField.currentText)
        }
    }
}
