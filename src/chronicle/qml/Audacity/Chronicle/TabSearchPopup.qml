/*
* Audacity: A Digital Audio Editor
*
* TabSearchPopup
*
* The searchable tab list behind the strip's overflow button. It offers the
* four searches the tab surface contract asks for, each with its own search
* bar and its own hook into the regular expression builder:
*
*   1. the current strip,
*   2. inside one group,
*   3. groups by name,
*   4. a master search across every stored strip.
*
* It is a popup window rather than an item inside the strip, so it is never
* clipped however narrow the strip is.
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

    signal tabChosen(string id, string uri)
    signal editGroupRequested(string groupId)
    signal regexBuilderRequested(string context)

    cornerRadius: M3.shape.medium
    elevationLevel: 3
    backgroundColor: M3.surfaceAt(3)
    borderColor: M3.color.outlineVariant

    contentWidth: 360
    contentHeight: 460

    property string scope: "strip"
    property string activeGroupId: ""

    property string stripQuery: ""
    property string groupQuery: ""
    property string groupsQuery: ""
    property string masterQuery: ""

    readonly property string currentQuery: {
        switch (root.scope) {
        case "group":
            return root.groupQuery
        case "groups":
            return root.groupsQuery
        case "all":
            return root.masterQuery
        default:
            return root.stripQuery
        }
    }

    // A query is treated as a regular expression when it compiles as one and
    // as plain text otherwise, so a typed bracket never empties the list.
    readonly property bool useRegex: root.tabModel ? root.tabModel.isValidRegex(root.currentQuery) : false

    readonly property var results: {
        if (!root.tabModel) {
            return []
        }
        switch (root.scope) {
        case "group":
            return root.tabModel.searchGroup(root.activeGroupId, root.groupQuery, root.useRegex)
        case "groups":
            return root.tabModel.searchGroups(root.groupsQuery, root.useRegex)
        case "all":
            return root.tabModel.searchAllStrips(root.masterQuery, root.useRegex)
        default:
            return root.tabModel.searchStrip(root.stripQuery, root.useRegex)
        }
    }

    NavigationPanel {
        id: navPanel

        name: "TabSearchPopup"
        enabled: root.isOpened
        direction: NavigationPanel.Vertical
    }

    Column {
        width: root.contentWidth
        spacing: 8

        M3SegmentedButton {
            id: scopeSelector

            width: parent.width

            model: [
                {
                    "text": qsTrc("chronicle", "This strip")
                },
                {
                    "text": qsTrc("chronicle", "In group")
                },
                {
                    "text": qsTrc("chronicle", "Groups")
                },
                {
                    "text": qsTrc("chronicle", "All strips")
                }
            ]

            currentIndex: 0
            navigationPanel: navPanel

            onActivated: function (index) {
                root.scope = ["strip", "group", "groups", "all"][index]
            }
        }

        // Each scope keeps its own search bar, so a term typed for one search
        // never silently narrows another.
        M3SearchBar {
            width: parent.width
            objectName: "TabSearchStripQuery"
            visible: root.scope === "strip"
            placeholder: qsTrc("chronicle", "Search tabs in this strip")
            accessibleName: qsTrc("chronicle", "Search tabs in this strip")
            showRegexBuilder: true
            searchText: root.stripQuery
            navigation.panel: navPanel
            onSearchTextChanged: root.stripQuery = searchText
            onRegexBuilderRequested: root.regexBuilderRequested("strip")
        }

        M3SearchBar {
            width: parent.width
            objectName: "TabSearchGroupQuery"
            visible: root.scope === "group"
            placeholder: qsTrc("chronicle", "Search tabs inside the group")
            accessibleName: qsTrc("chronicle", "Search tabs inside the group")
            showRegexBuilder: true
            searchText: root.groupQuery
            navigation.panel: navPanel
            onSearchTextChanged: root.groupQuery = searchText
            onRegexBuilderRequested: root.regexBuilderRequested("group")
        }

        M3SearchBar {
            width: parent.width
            objectName: "TabSearchGroupsQuery"
            visible: root.scope === "groups"
            placeholder: qsTrc("chronicle", "Search groups by name")
            accessibleName: qsTrc("chronicle", "Search groups by name")
            showRegexBuilder: true
            searchText: root.groupsQuery
            navigation.panel: navPanel
            onSearchTextChanged: root.groupsQuery = searchText
            onRegexBuilderRequested: root.regexBuilderRequested("groups")
        }

        M3SearchBar {
            width: parent.width
            objectName: "TabSearchAllQuery"
            visible: root.scope === "all"
            placeholder: qsTrc("chronicle", "Search every strip")
            accessibleName: qsTrc("chronicle", "Search every strip")
            showRegexBuilder: true
            searchText: root.masterQuery
            navigation.panel: navPanel
            onSearchTextChanged: root.masterQuery = searchText
            onRegexBuilderRequested: root.regexBuilderRequested("all")
        }

        // The group picker for the second search.
        Flow {
            width: parent.width
            visible: root.scope === "group"
            spacing: 6

            Repeater {
                model: root.tabModel ? root.tabModel.groups : []

                delegate: M3Chip {
                    required property var modelData

                    variant: "filter"
                    text: modelData.name + " (" + modelData.count + ")"
                    checked: modelData.id === root.activeGroupId
                    navigation.panel: navPanel

                    onToggled: root.activeGroupId = modelData.id
                }
            }
        }

        M3Divider {
            width: parent.width
        }

        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            visible: root.results.length === 0
            text: qsTrc("chronicle", "Nothing matches this search.")
            font: M3.typography.bodyMedium
            color: M3.color.onSurfaceVariant
        }

        StyledListView {
            width: parent.width
            height: 260

            model: root.results

            delegate: M3ListItem {
                id: resultItem

                required property int index
                required property var modelData

                width: ListView.view.width

                readonly property bool isGroup: root.scope === "groups"

                headline: resultItem.modelData.name !== undefined ? resultItem.modelData.name : resultItem.modelData.title
                supportingText: {
                    if (resultItem.isGroup) {
                        return qsTrc("chronicle", "%1 tabs").arg(resultItem.modelData.count)
                    }
                    if (resultItem.modelData.surfaceId !== undefined) {
                        return qsTrc("chronicle", "Strip %1").arg(resultItem.modelData.surfaceId)
                    }
                    return resultItem.modelData.kind
                }

                trailingText: resultItem.isGroup ? qsTrc("chronicle", "Edit group appearance…") : ""

                navigation.panel: navPanel
                navigation.row: resultItem.index

                onClicked: {
                    if (resultItem.isGroup) {
                        root.editGroupRequested(resultItem.modelData.id)
                        return
                    }
                    root.tabChosen(resultItem.modelData.id, resultItem.modelData.uri)
                }
            }
        }
    }
}
