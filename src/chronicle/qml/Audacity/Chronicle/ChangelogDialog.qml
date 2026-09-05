/*
* Audacity: A Digital Audio Editor
*
* ChangelogDialog
*
* The "What's new" dialog. It renders the release facing changelog one
* released version at a time. Every entry names the commit it describes and
* links to it, so a line in the changelog can always be traced back to the
* exact change in the repository.
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import Qt.labs.platform as Platform

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Chronicle

M3Dialog {
    id: root

    objectName: "ChangelogDialog"
    title: qsTrc("chronicle", "What's new")
    headline: qsTrc("chronicle", "What's new")
    supportingText: qsTrc("chronicle", "Every change in this and earlier releases, with a link to the commit it came from.")

    fullScreen: true

    // The hook the application's regular expression builder connects to.
    signal regexBuilderRequested

    ChangelogModel {
        id: changelogModel
    }

    Component.onCompleted: changelogModel.load()

    NavigationPanel {
        id: navPanel

        name: "ChangelogDialog"
        section: root.navigationSection
        direction: NavigationPanel.Vertical
        order: 1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            M3SearchBar {
                id: searchBar

                objectName: "ChangelogSearch"

                Layout.fillWidth: true
                placeholder: qsTrc("chronicle", "Search the changelog")
                accessibleName: qsTrc("chronicle", "Search the changelog")
                showRegexBuilder: true

                navigation.panel: navPanel
                navigation.order: 0

                onSearchTextChanged: changelogModel.searchText = searchBar.searchText
                onRegexBuilderRequested: root.regexBuilderRequested()
            }

            M3TextField {
                Layout.preferredWidth: 160
                label: qsTrc("chronicle", "From (YYYY-MM-DD)")
                currentText: changelogModel.fromDate
                navigation.panel: navPanel
                navigation.order: 1
                onTextEditingFinished: function (text) {
                    changelogModel.fromDate = text
                }
            }

            M3TextField {
                Layout.preferredWidth: 160
                label: qsTrc("chronicle", "To (YYYY-MM-DD)")
                currentText: changelogModel.toDate
                navigation.panel: navPanel
                navigation.order: 2
                onTextEditingFinished: function (text) {
                    changelogModel.toDate = text
                }
            }

            M3Button {
                text: qsTrc("chronicle", "Clear")
                variant: "text"
                navigation.panel: navPanel
                navigation.order: 3
                onClicked: {
                    searchBar.clear()
                    changelogModel.clearFilters()
                }
            }
        }

        StyledTextLabel {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.WordWrap
            visible: !changelogModel.available
            text: qsTrc("chronicle", "This build does not carry a changelog.")
            font: M3.typography.bodyMedium
            color: M3.color.onSurfaceVariant
        }

        StyledTextLabel {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignLeft
            visible: changelogModel.available && changelogModel.releases.length === 0
            text: qsTrc("chronicle", "Nothing in the changelog matches these filters.")
            font: M3.typography.bodyMedium
            color: M3.color.onSurfaceVariant
        }

        StyledFlickable {
            id: flickable

            Layout.fillWidth: true
            Layout.fillHeight: true

            contentWidth: width
            contentHeight: releasesColumn.implicitHeight

            Column {
                id: releasesColumn

                width: flickable.width
                spacing: 20

                Repeater {
                    model: changelogModel.releases

                    delegate: Column {
                        id: releaseBlock

                        required property int index
                        required property var modelData

                        width: releasesColumn.width
                        spacing: 8

                        StyledTextLabel {
                            width: parent.width
                            horizontalAlignment: Text.AlignLeft
                            text: releaseBlock.modelData.date !== "" ? releaseBlock.modelData.version + " — " + releaseBlock.modelData.date : releaseBlock.modelData.version
                            font: M3.typography.titleLarge
                            color: M3.color.onSurface
                        }

                        M3Divider {
                            width: parent.width
                        }

                        Repeater {
                            model: releaseBlock.modelData.entries

                            delegate: RowLayout {
                                id: entryRow

                                required property int index
                                required property var modelData

                                width: releaseBlock.width
                                spacing: 8

                                StyledTextLabel {
                                    Layout.preferredWidth: 96
                                    horizontalAlignment: Text.AlignLeft
                                    text: entryRow.modelData.group
                                    font: M3.typography.labelSmall
                                    color: M3.color.onSurfaceVariant
                                }

                                StyledTextLabel {
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignLeft
                                    wrapMode: Text.WordWrap
                                    text: entryRow.modelData.text
                                    font: M3.typography.bodyMedium
                                    color: M3.color.onSurface
                                }

                                M3Button {
                                    Layout.preferredWidth: 140
                                    variant: "text"
                                    text: entryRow.modelData.shortSha
                                    icon: IconCode.LINK
                                    accessibleName: qsTrc("chronicle", "Open commit %1").arg(entryRow.modelData.commitSha)
                                    toolTipTitle: entryRow.modelData.commitSha
                                    toolTipDescription: entryRow.modelData.commitUrl

                                    onClicked: Qt.openUrlExternally(entryRow.modelData.commitUrl)
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    actions: [
        M3Button {
            text: qsTrc("chronicle", "Export Markdown")
            variant: "text"
            onClicked: {
                exportDialog.format = "markdown"
                exportDialog.nameFilters = ["Markdown (*.md)"]
                exportDialog.open()
            }
        },
        M3Button {
            text: qsTrc("chronicle", "Export JSON")
            variant: "text"
            onClicked: {
                exportDialog.format = "json"
                exportDialog.nameFilters = ["JSON (*.json)"]
                exportDialog.open()
            }
        },
        M3Button {
            text: qsTrc("chronicle", "Export HTML")
            variant: "text"
            onClicked: {
                exportDialog.format = "html"
                exportDialog.nameFilters = ["HTML (*.html)"]
                exportDialog.open()
            }
        },
        M3Button {
            text: qsTrc("chronicle", "Close")
            variant: "filled"
            onClicked: root.hide()
        }
    ]

    Platform.FileDialog {
        id: exportDialog

        property string format: "markdown"

        title: qsTrc("chronicle", "Export the changelog")
        fileMode: Platform.FileDialog.SaveFile

        onAccepted: changelogModel.exportTo(exportDialog.currentFile.toString(), exportDialog.format)
    }
}
