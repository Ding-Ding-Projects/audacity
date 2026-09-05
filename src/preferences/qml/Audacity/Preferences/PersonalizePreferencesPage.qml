/*
* Audacity: A Digital Audio Editor
*
* PersonalizePreferencesPage
*
* Hosts the display name setting, the lock manager, the built in
* authenticator, and the joke support desk. Every lock created here is a
* self imposed speed bump, never a security boundary; the page says so.
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

    property int currentTab: 0
    readonly property var tabTitles: [qsTrc("preferences", "Rename"), qsTrc("preferences", "Locks"), qsTrc("preferences", "Authenticator"), qsTrc("preferences", "Support Tickets")]

    Column {
        id: content
        width: root.width
        spacing: 16
        padding: 24

        StyledTextLabel {
            text: qsTrc("preferences", "Personalize")
            font: M3.typography.headlineSmall
        }

        M3SegmentedButton {
            id: tabs
            width: parent.width - 48
            model: root.tabTitles
            currentIndex: root.currentTab
            onActivated: function (index) {
                root.currentTab = index
            }
        }

        // --- Rename ---
        Column {
            visible: root.currentTab === 0
            width: parent.width - 48
            spacing: 12

            StyledTextLabel {
                text: qsTrc("preferences", "This changes only what the title bar, About and notifications say. It never touches " + "this application's data folder, its installer identity or its update feed.")
                wrapMode: Text.WordWrap
                width: parent.width
                font: M3.typography.bodySmall
            }

            M3TextField {
                id: displayNameField
                width: parent.width
                label: qsTrc("preferences", "Application name")
                currentText: DisplayNameSettings.displayName
                onTextEditingFinished: function (text) {
                    DisplayNameSettings.displayName = text
                }
            }

            M3Button {
                text: qsTrc("preferences", "Reset to \"%1\"").arg(DisplayNameSettings.defaultDisplayName)
                variant: "outlined"
                onClicked: {
                    DisplayNameSettings.resetToDefault()
                    displayNameField.currentText = DisplayNameSettings.displayName
                }
            }
        }

        // --- Locks ---
        Column {
            visible: root.currentTab === 1
            width: parent.width - 48
            spacing: 12

            StyledTextLabel {
                text: qsTrc("preferences", "Toy locks are just for fun, never a security boundary. Recovery for any of them is " + "deleting this folder:")
                wrapMode: Text.WordWrap
                width: parent.width
                font: M3.typography.bodySmall
            }
            TextEdit {
                width: parent.width
                text: LockRegistry.dataFolderPath()
                readOnly: true
                selectByMouse: true
                wrapMode: Text.WrapAnywhere
            }

            M3TextField {
                id: lockSearch
                width: parent.width
                label: qsTrc("preferences", "Find a lock")
                placeholder: qsTrc("preferences", "Element name")
            }

            Repeater {
                model: LockRegistry.lockedElementIds()
                delegate: Row {
                    required property string modelData
                    width: content.width - 48
                    spacing: 8
                    visible: lockSearch.currentText === "" || modelData.toLowerCase().indexOf(lockSearch.currentText.toLowerCase()) >= 0

                    StyledTextLabel {
                        text: modelData
                        width: parent.width - 90
                        elide: Text.ElideMiddle
                    }
                    M3Button {
                        text: qsTrc("preferences", "Remove")
                        variant: "text"
                        onClicked: LockRegistry.removeLock(modelData)
                    }
                }
            }
        }

        // --- Authenticator ---
        Loader {
            visible: root.currentTab === 2
            active: root.currentTab === 2
            width: parent.width
            sourceComponent: authenticatorComponent
        }

        // --- Support Tickets ---
        Loader {
            visible: root.currentTab === 3
            active: root.currentTab === 3
            width: parent.width
            sourceComponent: supportComponent
        }
    }

    Component {
        id: authenticatorComponent
        AuthenticatorPage {
            width: content.width
            height: 480
        }
    }
    Component {
        id: supportComponent
        SupportTicketsPage {
            width: content.width
            height: 480
        }
    }
}
