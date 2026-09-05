/*
* Audacity: A Digital Audio Editor
*
* CloseTabsPopup
*
* "Close tabs containing text" and its inverse, "Close tabs not containing
* text".
*
* The query is plain text by default and a regular expression only when the
* regular expression switch is on. The command shows how many tabs it would
* close before it closes any, and it is disabled on an empty query and on a
* regular expression that does not compile, so it can never take every tab by
* accident. Pinned tabs are excluded unless the user asks for them.
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Chronicle

StyledPopupView {
    id: root

    property TabStripModel tabModel: null

    // True for "containing", false for the inverse.
    property bool containing: true

    signal regexBuilderRequested

    cornerRadius: M3.shape.large
    elevationLevel: 3
    backgroundColor: M3.surfaceAt(3)
    borderColor: M3.color.outlineVariant

    contentWidth: 380
    contentHeight: 420

    property string query: ""
    property bool useRegex: false
    property bool includePinned: false

    readonly property bool regexBroken: root.useRegex && root.query !== "" && root.tabModel && !root.tabModel.isValidRegex(root.query)

    readonly property var victims: {
        if (!root.tabModel || root.query === "" || root.regexBroken) {
            return []
        }
        return root.tabModel.closePreview(root.query, root.useRegex, root.containing, root.includePinned)
    }

    readonly property bool canApply: root.query !== "" && !root.regexBroken && root.victims.length > 0

    NavigationPanel {
        id: navPanel

        name: "CloseTabs"
        enabled: root.isOpened
        direction: NavigationPanel.Vertical
    }

    Column {
        width: root.contentWidth
        spacing: 12

        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            text: root.containing ? qsTrc("chronicle", "Close tabs containing text") : qsTrc("chronicle", "Close tabs not containing text")
            font: M3.typography.titleMedium
            color: M3.color.onSurface
        }

        M3SearchBar {
            id: queryField

            objectName: "CloseTabsQuery"

            width: parent.width
            placeholder: qsTrc("chronicle", "Text to match")
            accessibleName: qsTrc("chronicle", "Text to match")
            showRegexBuilder: true
            navigation.panel: navPanel

            onSearchTextChanged: root.query = queryField.searchText
            onRegexBuilderRequested: root.regexBuilderRequested()
        }

        M3Switch {
            text: qsTrc("chronicle", "Treat the text as a regular expression")
            checked: root.useRegex
            navigation.panel: navPanel
            onToggled: function (checked) {
                root.useRegex = checked
            }
        }

        M3Switch {
            text: qsTrc("chronicle", "Include pinned tabs")
            checked: root.includePinned
            navigation.panel: navPanel
            onToggled: function (checked) {
                root.includePinned = checked
            }
        }

        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            visible: root.regexBroken
            wrapMode: Text.WordWrap
            text: qsTrc("chronicle", "This is not a valid regular expression, so nothing will be closed.")
            font: M3.typography.bodySmall
            color: M3.color.error
        }

        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            text: {
                if (root.query === "") {
                    return qsTrc("chronicle", "Type some text to see what would be closed.")
                }
                if (root.regexBroken) {
                    return ""
                }
                return qsTrc("chronicle", "This would close %1 of %2 tabs.").arg(root.victims.length).arg(root.tabModel ? root.tabModel.tabs.length : 0)
            }
            font: M3.typography.bodyMedium
            color: M3.color.onSurfaceVariant
        }

        StyledListView {
            width: parent.width
            height: 140

            model: root.victims

            delegate: M3ListItem {
                required property var modelData

                width: ListView.view.width
                clickable: false
                headline: modelData.title
                supportingText: modelData.kind
            }
        }

        Row {
            spacing: 8

            M3Button {
                text: qsTrc("chronicle", "Cancel")
                variant: "text"
                navigation.panel: navPanel
                onClicked: root.close()
            }

            M3Button {
                text: qsTrc("chronicle", "Close tabs")
                variant: "filled"
                enabled: root.canApply
                navigation.panel: navPanel

                onClicked: {
                    root.tabModel.applyClose(root.query, root.useRegex, root.containing, root.includePinned)
                    root.close()
                }
            }
        }
    }
}
