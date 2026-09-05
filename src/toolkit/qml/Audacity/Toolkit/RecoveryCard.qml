/*
* Audacity: A Digital Audio Editor
*
* RecoveryCard
*
* A reusable failure recovery presentation. Placed beside any operation that
* can fail for reasons the user cannot diagnose from the error alone: it
* offers Retry, Open logs folder and Copy diagnostic, right where the
* failure happened rather than in a menu elsewhere.
*
* API:
*     message, diagnosticText, logsFolderPath
*     retryRequested(), openLogsRequested(), diagnosticCopied()
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Rectangle {
    id: root

    property string message: qsTrc("toolkit", "That did not work.")
    property string diagnosticText: ""
    property string logsFolderPath: ""

    signal retryRequested
    signal openLogsRequested
    signal diagnosticCopied

    Accessible.role: Accessible.AlertMessage
    Accessible.name: message

    radius: M3.shape.medium
    color: M3.color.errorContainer
    border.width: 1
    border.color: M3.color.error

    implicitHeight: content.implicitHeight + 24

    ColumnLayout {
        id: content

        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        StyledTextLabel {
            Layout.fillWidth: true
            text: root.message
            wrapMode: Text.WordWrap
            font: ui.theme.bodyFont
        }

        RowLayout {
            spacing: 8

            M3Button {
                text: qsTrc("toolkit", "Retry")
                accessibleName: qsTrc("toolkit", "Retry the failed operation")
                variant: "outlined"
                onClicked: root.retryRequested()
            }

            M3Button {
                text: qsTrc("toolkit", "Open logs folder")
                accessibleName: qsTrc("toolkit", "Open the folder holding the diagnostic logs")
                variant: "outlined"
                visible: root.logsFolderPath.length > 0
                onClicked: root.openLogsRequested()
            }

            M3Button {
                text: qsTrc("toolkit", "Copy diagnostic")
                accessibleName: qsTrc("toolkit", "Copy the diagnostic details to the clipboard")
                variant: "text"
                onClicked: {
                    diagnosticField.selectAll()
                    diagnosticField.copy()
                    root.diagnosticCopied()
                }
            }
        }

        TextInput {
            id: diagnosticField

            visible: false
            text: root.diagnosticText
        }
    }
}
