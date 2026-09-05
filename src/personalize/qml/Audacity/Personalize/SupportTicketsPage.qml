/*
* Audacity: A Digital Audio Editor
*
* SupportTicketsPage
*
* The joke support desk. Plays the part properly, right down to a made up
* ticket number and a canned response, then its one real action is opening
* the application's own data folder so a locked out person can delete it
* themselves. Nothing is sent anywhere; the disclosure below says so in
* plain, unstyled words regardless of the active funny level.
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

    SupportTickets {
        id: tickets
    }

    Column {
        id: content
        width: root.width
        spacing: 16
        padding: 24

        StyledTextLabel {
            text: qsTrc("personalize", "Support Tickets")
            font: M3.typography.headlineSmall
        }

        Rectangle {
            width: parent.width - 48
            radius: M3.shape.medium
            color: M3.color.errorContainer
            implicitHeight: disclosure.implicitHeight + 24

            StyledTextLabel {
                id: disclosure
                anchors.fill: parent
                anchors.margins: 12
                wrapMode: Text.WordWrap
                text: "Nothing on this page is sent anywhere. No ticket leaves this machine, no network " + "request is made, and nobody is reading it. This is a joke desk whose only real " + "action is opening your own application data folder so you can delete it yourself."
            }
        }

        M3TextField {
            id: categoryField
            width: parent.width - 48
            label: qsTrc("personalize", "Category")
            currentText: qsTrc("personalize", "Locked out")
        }
        M3TextField {
            id: descriptionField
            width: parent.width - 48
            label: qsTrc("personalize", "Describe the problem")
        }

        M3Button {
            text: qsTrc("personalize", "Open a ticket")
            variant: "filled"
            onClicked: {
                var id = tickets.openTicket(categoryField.currentText, descriptionField.currentText)
                ticketList.model = tickets.tickets()
            }
        }

        Repeater {
            id: ticketList
            model: tickets.tickets()
            delegate: Column {
                required property var modelData
                width: content.width - 48
                spacing: 4

                Rectangle {
                    width: parent.width
                    radius: M3.shape.medium
                    color: M3.color.surfaceContainer
                    implicitHeight: ticketColumn.implicitHeight + 24

                    Column {
                        id: ticketColumn
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 4

                        StyledTextLabel {
                            text: qsTrc("personalize", "Ticket %1, resolved").arg(modelData.id)
                            font: M3.typography.titleSmall
                        }
                        StyledTextLabel {
                            text: modelData.description
                            wrapMode: Text.WordWrap
                            width: parent.width
                        }
                        StyledTextLabel {
                            text: modelData.response
                            wrapMode: Text.WordWrap
                            width: parent.width
                            font: M3.typography.bodySmall
                        }
                        TextEdit {
                            width: parent.width
                            text: tickets.dataFolderPath
                            readOnly: true
                            selectByMouse: true
                            wrapMode: Text.WrapAnywhere
                            color: M3.color.onSurfaceVariant
                            font: M3.typography.bodySmall
                            Accessible.name: qsTrc("personalize", "The application data folder path, selectable to copy")
                        }
                        M3Button {
                            text: qsTrc("personalize", "Open the application data folder")
                            variant: "outlined"
                            onClicked: tickets.openDataFolder()
                        }
                    }
                }
            }
        }
    }
}
