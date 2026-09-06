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
                            text: revisionRow.modelData.starred
                                  ? qsTrc("chronicle", "Unstar")
                                  : qsTrc("chronicle", "Star")
                            variant: revisionRow.modelData.starred ? "filled" : "text"
                            accessible.checked: revisionRow.modelData.starred
                            onClicked: historyModel.setStarred(revisionRow.modelData.revisionId, !revisionRow.modelData.starred)
                        }

                        M3Button {
                            text: revisionRow.modelData.pinned
                                  ? qsTrc("chronicle", "Unpin")
                                  : qsTrc("chronicle", "Pin")
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
}
