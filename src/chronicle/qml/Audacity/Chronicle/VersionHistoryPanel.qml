/*
* Audacity: A Digital Audio Editor
*
* VersionHistoryPanel
*
* The local version history: every revision the application recorded, with the
* time it was taken, the label describing what changed and a chip naming the
* action that triggered it.
*
* A revision can be browsed, its file list inspected, its label edited, its
* content exported to a folder and its content restored. A restore is itself
* recorded as a new revision, so nothing in the history is ever rewritten.
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.platform as Platform

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Companion
import Audacity.Chronicle

Item {
    id: root

    property alias navigationSection: navPanel.section
    property alias navigationOrderStart: navPanel.order

    // The hook the application's regular expression builder connects to.
    signal regexBuilderRequested

    VersionHistoryModel {
        id: historyModel
    }

    Component.onCompleted: historyModel.load()

    NavigationPanel {
        id: navPanel

        name: "VersionHistoryPanel"
        direction: NavigationPanel.Vertical
        enabled: root.enabled && root.visible
        accessible.name: qsTrc("chronicle", "Version history")
    }

    QtObject {
        id: prv

        property string selectedRevisionId: ""
        property bool filtersExpanded: false
        property bool editingLabel: false

        // Local search state for the three surfaces below, each isolated
        // from the panel's own main search field and from each other.
        property string timelineSearchText: ""
        property string storageSearchText: ""
        property string compareSearchText: ""
        property string compareRevisionIdA: ""
        property string compareRevisionIdB: ""

        // Used as a regular expression when it compiles as one and as plain
        // text otherwise, exactly like the panel's main search field, so a
        // typed bracket never empties a list here either.
        function matches(haystack, needle) {
            if (needle.length === 0) {
                return true
            }
            try {
                var expression = new RegExp(needle, "i")
                return expression.test(haystack)
            } catch (invalidPattern) {
                return haystack.toLowerCase().indexOf(needle.toLowerCase()) !== -1
            }
        }

        function filteredDayGroups(groups, needle) {
            var result = []
            for (var i = 0; i < groups.length; i++) {
                if (prv.matches(groups[i].date, needle)) {
                    result.push(groups[i])
                }
            }
            return result
        }

        function storageRows(info) {
            return [
                {
                    "label": qsTrc("chronicle", "Backend"),
                    "value": info.backend === "git" ? qsTrc("chronicle", "Git repository") : qsTrc("chronicle", "Content addressed store")
                },
                {
                    "label": qsTrc("chronicle", "Repository size"),
                    "value": prv.formatSize(info.repositoryBytes)
                },
                {
                    "label": qsTrc("chronicle", "Revisions recorded"),
                    "value": String(info.revisionCount)
                }
            ]
        }

        function filteredStorageRows(info, needle) {
            var rows = prv.storageRows(info)
            var result = []
            for (var i = 0; i < rows.length; i++) {
                if (prv.matches(rows[i].label, needle)) {
                    result.push(rows[i])
                }
            }
            return result
        }

        function filteredRevisionsForCompare(revisions, needle) {
            var result = []
            for (var i = 0; i < revisions.length; i++) {
                var haystack = revisions[i].label + " " + revisions[i].actionTitle + " " + revisions[i].timestamp
                if (prv.matches(haystack, needle)) {
                    result.push(revisions[i])
                }
            }
            return result
        }

        function formatSize(bytes) {
            if (bytes < 1024) {
                return qsTrc("chronicle", "%1 B").arg(bytes)
            }
            if (bytes < 1024 * 1024) {
                return qsTrc("chronicle", "%1 kB").arg((bytes / 1024).toFixed(1))
            }
            return qsTrc("chronicle", "%1 MB").arg((bytes / (1024 * 1024)).toFixed(1))
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Search.
        M3SearchBar {
            id: searchBar

            objectName: "VersionHistorySearch"

            Layout.fillWidth: true
            Layout.margins: 12

            placeholder: qsTrc("chronicle", "Search the version history")
            accessibleName: qsTrc("chronicle", "Search the version history")
            showRegexBuilder: true

            navigation.panel: navPanel
            navigation.order: 0

            onSearchTextChanged: historyModel.searchText = searchBar.searchText
            onRegexBuilderRequested: {
                regexBuilder.pattern = searchBar.searchText
                regexBuilder.open()
                root.regexBuilderRequested()
            }
        }

        // Action filter chips, derived from the actions actually recorded.
        Flow {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: 8
            spacing: 6

            Repeater {
                model: historyModel.actionCounts

                delegate: M3Chip {
                    id: actionChip

                    required property int index
                    required property var modelData

                    variant: "filter"
                    text: actionChip.modelData.title + " (" + actionChip.modelData.count + ")"
                    checked: historyModel.selectedActions.indexOf(actionChip.modelData.action) !== -1

                    navigation.panel: navPanel
                    navigation.order: 1 + actionChip.index

                    onToggled: function (checked) {
                        var selection = historyModel.selectedActions.slice()
                        var at = selection.indexOf(actionChip.modelData.action)
                        if (checked && at === -1) {
                            selection.push(actionChip.modelData.action)
                        } else if (!checked && at !== -1) {
                            selection.splice(at, 1)
                        }
                        historyModel.selectedActions = selection
                    }
                }
            }
        }

        // Timeline rail: one chip per calendar day that has a revision,
        // oldest first, with its own local search, isolated from the
        // panel's main search field above.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: 8
            spacing: 6

            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("chronicle", "Timeline")
                font: M3.typography.labelLarge
                color: M3.color.onSurfaceVariant
            }

            M3SearchBar {
                id: timelineSearchBar

                objectName: "VersionHistoryTimelineSearch"
                Layout.fillWidth: true

                placeholder: qsTrc("chronicle", "Search the timeline by day")
                accessibleName: qsTrc("chronicle", "Search the timeline by day")
                showRegexBuilder: true

                navigation.panel: navPanel
                navigation.order: 30

                onSearchTextChanged: prv.timelineSearchText = timelineSearchBar.searchText
                onRegexBuilderRequested: {
                    timelineRegexBuilder.pattern = timelineSearchBar.searchText
                    timelineRegexBuilder.open()
                }
            }

            Flow {
                Layout.fillWidth: true
                spacing: 6

                Repeater {
                    model: prv.filteredDayGroups(historyModel.dayGroups(), prv.timelineSearchText)

                    delegate: M3Chip {
                        id: dayChip

                        required property var modelData

                        variant: "assist"
                        text: dayChip.modelData.date + " (" + dayChip.modelData.count + ")"
                        accessibleName: qsTrc("chronicle", "Show revisions from %1").arg(dayChip.modelData.date)

                        navigation.panel: navPanel
                        navigation.order: 31

                        onClicked: {
                            historyModel.fromDate = dayChip.modelData.date
                            historyModel.toDate = dayChip.modelData.date
                        }
                    }
                }
            }
        }

        // The date range, with presets, typed ISO dates and an anchored
        // calendar for each end of the range.
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            spacing: 8

            M3TextField {
                id: fromField

                Layout.fillWidth: true
                label: qsTrc("chronicle", "From (YYYY-MM-DD)")
                currentText: historyModel.fromDate
                trailingIcon: IconCode.CLOCK
                navigation.panel: navPanel
                navigation.order: 40

                onTextEditingFinished: function (text) {
                    historyModel.fromDate = text
                }
            }

            M3IconButton {
                icon: IconCode.CLOCK
                accessibleName: qsTrc("chronicle", "Pick the start of the range")
                navigation.panel: navPanel
                navigation.order: 41

                onClicked: {
                    fromPicker.parent = fromField
                    fromPicker.open()
                }
            }

            M3TextField {
                id: toField

                Layout.fillWidth: true
                label: qsTrc("chronicle", "To (YYYY-MM-DD)")
                currentText: historyModel.toDate
                navigation.panel: navPanel
                navigation.order: 42

                onTextEditingFinished: function (text) {
                    historyModel.toDate = text
                }
            }

            M3IconButton {
                icon: IconCode.CLOCK
                accessibleName: qsTrc("chronicle", "Pick the end of the range")
                navigation.panel: navPanel
                navigation.order: 43

                onClicked: {
                    toPicker.parent = toField
                    toPicker.open()
                }
            }
        }

        // Range presets.
        Flow {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 6

            Repeater {
                model: [
                    {
                        "title": qsTrc("chronicle", "Today"),
                        "days": 0
                    },
                    {
                        "title": qsTrc("chronicle", "Last 7 days"),
                        "days": 7
                    },
                    {
                        "title": qsTrc("chronicle", "Last 30 days"),
                        "days": 30
                    },
                    {
                        "title": qsTrc("chronicle", "All time"),
                        "days": -1
                    }
                ]

                delegate: M3Chip {
                    id: presetChip

                    required property int index
                    required property var modelData

                    variant: "assist"
                    text: presetChip.modelData.title
                    navigation.panel: navPanel
                    navigation.order: 50 + presetChip.index

                    onClicked: {
                        if (presetChip.modelData.days < 0) {
                            historyModel.fromDate = ""
                            historyModel.toDate = ""
                            return
                        }
                        var to = new Date()
                        var from = new Date()
                        from.setDate(to.getDate() - presetChip.modelData.days)
                        historyModel.fromDate = Qt.formatDate(from, "yyyy-MM-dd")
                        historyModel.toDate = Qt.formatDate(to, "yyyy-MM-dd")
                    }
                }
            }
        }

        M3Divider {
            Layout.fillWidth: true
        }

        StyledTextLabel {
            Layout.fillWidth: true
            Layout.margins: 12
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            visible: historyModel.revisions.length === 0
            text: qsTrc("chronicle", "No revision matches these filters yet. The history records a revision " + "when the project is saved, when a setting changes and when a preset is saved or deleted.")
            font: M3.typography.bodyMedium
            color: M3.color.onSurfaceVariant
        }

        StyledListView {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true

            model: historyModel.revisions
            scrollBarPolicy: ScrollBar.AsNeeded

            delegate: Column {
                id: revisionRow

                required property int index
                required property var modelData

                readonly property bool expanded: revisionRow.modelData.revisionId === prv.selectedRevisionId

                width: listView.width

                M3ListItem {
                    width: parent.width

                    overline: revisionRow.modelData.timestamp
                    headline: revisionRow.modelData.label
                    supportingText: revisionRow.modelData.actionTitle + " · " + revisionRow.modelData.shortId
                    selected: revisionRow.expanded
                    leadingIcon: revisionRow.expanded ? IconCode.SMALL_ARROW_DOWN : IconCode.SMALL_ARROW_RIGHT

                    accessibleName: revisionRow.modelData.label + ", " + revisionRow.modelData.actionTitle + ", " + revisionRow.modelData.timestamp

                    navigation.panel: navPanel
                    navigation.order: 100 + revisionRow.index

                    onClicked: {
                        prv.editingLabel = false
                        prv.selectedRevisionId = revisionRow.expanded ? "" : revisionRow.modelData.revisionId
                    }
                }

                // The revision detail: the diff summary and the actions.
                Column {
                    width: parent.width - 24
                    x: 24
                    spacing: 8
                    visible: revisionRow.expanded

                    StyledTextLabel {
                        width: parent.width
                        horizontalAlignment: Text.AlignLeft
                        text: qsTrc("chronicle", "Files in this revision")
                        font: M3.typography.labelLarge
                        color: M3.color.onSurfaceVariant
                    }

                    Repeater {
                        model: revisionRow.expanded ? historyModel.filesOf(revisionRow.modelData.revisionId) : []

                        delegate: RowLayout {
                            required property var modelData

                            width: parent.width
                            spacing: 8

                            StyledTextLabel {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignLeft
                                elide: Text.ElideMiddle
                                text: modelData.path
                                font: M3.typography.bodySmall
                                color: M3.color.onSurface
                            }

                            StyledTextLabel {
                                horizontalAlignment: Text.AlignRight
                                text: modelData.status
                                font: M3.typography.labelSmall
                                color: M3.color.onSurfaceVariant
                            }

                            StyledTextLabel {
                                horizontalAlignment: Text.AlignRight
                                text: prv.formatSize(modelData.size)
                                font: M3.typography.labelSmall
                                color: M3.color.onSurfaceVariant
                            }
                        }
                    }

                    M3TextField {
                        id: labelField

                        width: parent.width
                        visible: prv.editingLabel
                        label: qsTrc("chronicle", "Label")
                        currentText: revisionRow.modelData.label

                        onTextEditingFinished: function (text) {
                            historyModel.setLabel(revisionRow.modelData.revisionId, text)
                            prv.editingLabel = false
                        }
                    }

                    Flow {
                        width: parent.width
                        spacing: 8

                        M3Button {
                            text: qsTrc("chronicle", "Restore")
                            variant: "filled"
                            onClicked: historyModel.restore(revisionRow.modelData.revisionId)
                        }

                        M3Button {
                            text: qsTrc("chronicle", "Edit label")
                            variant: "outlined"
                            onClicked: prv.editingLabel = !prv.editingLabel
                        }

                        M3Button {
                            text: qsTrc("chronicle", "Export…")
                            variant: "text"
                            onClicked: exportDialog.open()
                        }

                        M3Button {
                            text: qsTrc("chronicle", "Open as new project")
                            variant: "text"
                            accessible.description: qsTrc("chronicle", "Opens this revision without touching the project you have open")
                            onClicked: historyModel.openAsNewProject(revisionRow.modelData.revisionId)
                        }

                        M3Button {
                            text: revisionRow.modelData.starred ? qsTrc("chronicle", "Unstar") : qsTrc("chronicle", "Star")
                            variant: revisionRow.modelData.starred ? "filled" : "text"
                            accessible.checked: revisionRow.modelData.starred
                            onClicked: historyModel.setStarred(revisionRow.modelData.revisionId, !revisionRow.modelData.starred)
                        }

                        M3Button {
                            text: revisionRow.modelData.pinned ? qsTrc("chronicle", "Unpin") : qsTrc("chronicle", "Pin")
                            variant: revisionRow.modelData.pinned ? "filled" : "text"
                            accessible.checked: revisionRow.modelData.pinned
                            accessible.description: qsTrc("chronicle", "A pinned revision is never removed by retention")
                            onClicked: historyModel.setPinned(revisionRow.modelData.revisionId, !revisionRow.modelData.pinned)
                        }
                    }
                }
            }
        }

        M3Divider {
            Layout.fillWidth: true
        }

        // Retention.
        RowLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 8

            M3TextField {
                Layout.fillWidth: true
                label: qsTrc("chronicle", "Keep at most")
                currentText: String(historyModel.retentionCount)
                supportingText: qsTrc("chronicle", "revisions")
                navigation.panel: navPanel
                navigation.order: 9000

                onTextEditingFinished: function (text) {
                    historyModel.retentionCount = parseInt(text, 10)
                }
            }

            M3TextField {
                Layout.fillWidth: true
                label: qsTrc("chronicle", "Keep for")
                currentText: String(historyModel.retentionDays)
                supportingText: qsTrc("chronicle", "days")
                navigation.panel: navPanel
                navigation.order: 9001

                onTextEditingFinished: function (text) {
                    historyModel.retentionDays = parseInt(text, 10)
                }
            }

            M3Button {
                text: qsTrc("chronicle", "Apply retention")
                variant: "outlined"
                navigation.panel: navPanel
                navigation.order: 9002
                onClicked: historyModel.prune()
            }
        }

        M3Divider {
            Layout.fillWidth: true
        }

        // Storage: repository size on disk, backend and revision count, each
        // row filterable by its own local search.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 6

            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("chronicle", "Storage")
                font: M3.typography.labelLarge
                color: M3.color.onSurfaceVariant
            }

            M3SearchBar {
                id: storageSearchBar

                objectName: "VersionHistoryStorageSearch"
                Layout.fillWidth: true

                placeholder: qsTrc("chronicle", "Search storage details")
                accessibleName: qsTrc("chronicle", "Search storage details")
                showRegexBuilder: true

                navigation.panel: navPanel
                navigation.order: 9010

                onSearchTextChanged: prv.storageSearchText = storageSearchBar.searchText
                onRegexBuilderRequested: {
                    storageRegexBuilder.pattern = storageSearchBar.searchText
                    storageRegexBuilder.open()
                }
            }

            M3Card {
                Layout.fillWidth: true
                variant: "outlined"
                accessibleName: qsTrc("chronicle", "Storage details")

                ColumnLayout {
                    width: parent.width
                    spacing: 4

                    Repeater {
                        model: prv.filteredStorageRows(historyModel.storageInfo(), prv.storageSearchText)

                        delegate: RowLayout {
                            required property var modelData

                            Layout.fillWidth: true

                            StyledTextLabel {
                                Layout.fillWidth: true
                                horizontalAlignment: Text.AlignLeft
                                text: modelData.label
                                font: M3.typography.bodyMedium
                                color: M3.color.onSurfaceVariant
                            }

                            StyledTextLabel {
                                horizontalAlignment: Text.AlignRight
                                text: modelData.value
                                font: M3.typography.bodyMedium
                                color: M3.color.onSurface
                            }
                        }
                    }
                }
            }
        }

        M3Divider {
            Layout.fillWidth: true
        }

        // Compare: pick two revisions from one filterable local list, then
        // see how their recorded files differ by path and size. This does
        // not compare track count, clip count or sample rate, which this
        // history does not capture per revision.
        ColumnLayout {
            Layout.fillWidth: true
            Layout.margins: 12
            spacing: 6

            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                text: qsTrc("chronicle", "Compare two revisions")
                font: M3.typography.labelLarge
                color: M3.color.onSurfaceVariant
            }

            M3SearchBar {
                id: compareSearchBar

                objectName: "VersionHistoryCompareSearch"
                Layout.fillWidth: true

                placeholder: qsTrc("chronicle", "Search revisions to compare")
                accessibleName: qsTrc("chronicle", "Search revisions to compare")
                showRegexBuilder: true

                navigation.panel: navPanel
                navigation.order: 9020

                onSearchTextChanged: prv.compareSearchText = compareSearchBar.searchText
                onRegexBuilderRequested: {
                    compareRegexBuilder.pattern = compareSearchBar.searchText
                    compareRegexBuilder.open()
                }
            }

            Repeater {
                model: prv.filteredRevisionsForCompare(historyModel.revisions, prv.compareSearchText)

                delegate: RowLayout {
                    id: compareRow

                    required property var modelData

                    Layout.fillWidth: true
                    spacing: 6

                    StyledTextLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        elide: Text.ElideRight
                        text: compareRow.modelData.label + " · " + compareRow.modelData.timestamp
                        font: M3.typography.bodyMedium
                        color: M3.color.onSurface
                    }

                    M3Button {
                        text: prv.compareRevisionIdA === compareRow.modelData.revisionId ? qsTrc("chronicle", "Is A") : qsTrc("chronicle", "Set as A")
                        variant: prv.compareRevisionIdA === compareRow.modelData.revisionId ? "filled" : "outlined"
                        accessible.checked: prv.compareRevisionIdA === compareRow.modelData.revisionId
                        onClicked: prv.compareRevisionIdA = compareRow.modelData.revisionId
                    }

                    M3Button {
                        text: prv.compareRevisionIdB === compareRow.modelData.revisionId ? qsTrc("chronicle", "Is B") : qsTrc("chronicle", "Set as B")
                        variant: prv.compareRevisionIdB === compareRow.modelData.revisionId ? "filled" : "outlined"
                        accessible.checked: prv.compareRevisionIdB === compareRow.modelData.revisionId
                        onClicked: prv.compareRevisionIdB = compareRow.modelData.revisionId
                    }
                }
            }

            M3Card {
                Layout.fillWidth: true
                variant: "outlined"
                visible: prv.compareRevisionIdA.length > 0 && prv.compareRevisionIdB.length > 0
                accessibleName: qsTrc("chronicle", "Comparison result")

                ColumnLayout {
                    width: parent.width
                    spacing: 4

                    property var comparison: (prv.compareRevisionIdA.length > 0 && prv.compareRevisionIdB.length > 0) ? historyModel.compareRevisions(prv.compareRevisionIdA, prv.compareRevisionIdB) : ({})

                    StyledTextLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        text: qsTrc("chronicle", "%1 file(s) added, %2 modified, %3 deleted, %4 unchanged").arg(parent.comparison.filesAdded || 0).arg(parent.comparison.filesModified || 0).arg(parent.comparison.filesDeleted || 0).arg(parent.comparison.filesUnchanged || 0)
                        font: M3.typography.bodyMedium
                        color: M3.color.onSurface
                        wrapMode: Text.WordWrap
                    }

                    StyledTextLabel {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignLeft
                        text: qsTrc("chronicle", "Recorded size: %1 then %2").arg(prv.formatSize(parent.comparison.totalBytesA || 0)).arg(prv.formatSize(parent.comparison.totalBytesB || 0))
                        font: M3.typography.bodySmall
                        color: M3.color.onSurfaceVariant
                    }
                }
            }
        }

        StyledTextLabel {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.bottomMargin: 12
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            text: historyModel.storeKind === "git" ? qsTrc("chronicle", "The history is kept in a local Git repository beside the application data.") : qsTrc("chronicle", "Git was not found, so the history is kept in the built in content addressed store.")
            font: M3.typography.bodySmall
            color: M3.color.onSurfaceVariant
        }
    }

    StyledPopupView {
        id: fromPicker

        contentWidth: 328
        contentHeight: 456
        backgroundColor: M3.surfaceAt(3)

        M3DatePicker {
            width: fromPicker.contentWidth
            height: fromPicker.contentHeight

            onDateSelected: function (value) {
                historyModel.fromDate = Qt.formatDate(value, "yyyy-MM-dd")
                fromPicker.close()
            }
        }
    }

    StyledPopupView {
        id: toPicker

        contentWidth: 328
        contentHeight: 456
        backgroundColor: M3.surfaceAt(3)

        M3DatePicker {
            width: toPicker.contentWidth
            height: toPicker.contentHeight

            onDateSelected: function (value) {
                historyModel.toDate = Qt.formatDate(value, "yyyy-MM-dd")
                toPicker.close()
            }
        }
    }

    Platform.FolderDialog {
        id: exportDialog

        title: qsTrc("chronicle", "Export the revision to a folder")

        onAccepted: {
            historyModel.exportRevision(prv.selectedRevisionId, exportDialog.currentFolder.toString())
        }
    }
    // The regular expression builder for this field. Each search surface owns
    // its own instance, so its pattern, flags, sample and saved test cases are
    // isolated from every other search field in the application.
    RegexBuilderSheet {
        id: regexBuilder

        anchors.fill: parent

        storeName: "version-history"
        fieldLabel: "Version history"

        onPatternAccepted: function (pattern) {
            searchBar.searchText = pattern
        }
    }

    RegexBuilderSheet {
        id: timelineRegexBuilder

        anchors.fill: parent

        storeName: "version-history-timeline"
        fieldLabel: "Version history timeline"

        onPatternAccepted: function (pattern) {
            timelineSearchBar.searchText = pattern
        }
    }

    RegexBuilderSheet {
        id: storageRegexBuilder

        anchors.fill: parent

        storeName: "version-history-storage"
        fieldLabel: "Version history storage"

        onPatternAccepted: function (pattern) {
            storageSearchBar.searchText = pattern
        }
    }

    RegexBuilderSheet {
        id: compareRegexBuilder

        anchors.fill: parent

        storeName: "version-history-compare"
        fieldLabel: "Version history compare"

        onPatternAccepted: function (pattern) {
            compareSearchBar.searchText = pattern
        }
    }
}
