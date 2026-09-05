/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Experience

// The side sheet that lists every notification, including the dismissed ones.
// Its search field carries the shared regular expression builder.
M3SideSheet {
    id: root

    signal regexBuilderRequested(var searchBar)

    headline: qsTrc("experience", "Notification centre")
    sheetWidth: 400
    modal: true

    NotificationListModel {
        id: historyModel

        historyMode: true

        Component.onCompleted: historyModel.load()
    }

    Column {
        anchors.fill: parent
        spacing: 12

        M3SearchBar {
            id: searchBar

            objectName: "NotificationCentreSearch"

            width: parent.width
            placeholder: qsTrc("experience", "Search notifications")
            accessibleName: qsTrc("experience", "Search notifications")
            showRegexBuilder: true

            onSearchTextChanged: historyModel.searchText = searchBar.searchText
            onRegexBuilderRequested: root.regexBuilderRequested(searchBar)
        }

        Row {
            spacing: 8

            M3Button {
                text: qsTrc("experience", "Dismiss all")
                variant: "outlined"

                onClicked: historyModel.dismissAll()
            }

            M3Button {
                text: qsTrc("experience", "Clear list")
                variant: "text"

                onClicked: historyModel.clearHistory()
            }

            M3Button {
                text: qsTrc("experience", "Export as JSON")
                variant: "text"
                enabled: historyModel.count > 0
                accessibleName: qsTrc("experience", "Export the notifications shown here as JSON, copied to the clipboard")

                onClicked: {
                    exportClipboard.text = historyModel.exportJson()
                    exportClipboard.selectAll()
                    exportClipboard.copy()
                }
            }
        }

        // A hidden field is the least surprising way to reach the system
        // clipboard from QML: it never becomes visible, and it never takes
        // keyboard focus away from the search field above it.
        TextEdit {
            id: exportClipboard

            visible: false
        }

        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            visible: historyModel.count === 0
            text: searchBar.searchText === "" ? qsTrc("experience", "Nothing has been reported yet.") : qsTrc("experience", "Nothing matches this search.")
            color: M3.color.onSurfaceVariant
        }

        StyledListView {
            width: parent.width
            height: root.height - y - 24
            spacing: 4

            model: historyModel

            delegate: M3ListItem {
                id: item

                required property var model

                width: ListView.view.width

                overline: item.model.timeText
                headline: item.model.title
                supportingText: item.model.body
                clickable: item.model.actionText !== ""
                trailingText: item.model.dismissed ? "" : qsTrc("experience", "On screen")
                accessibleName: item.model.title + ". " + item.model.body

                onClicked: historyModel.triggerAction(item.model.notificationId)
            }
        }
    }
}
