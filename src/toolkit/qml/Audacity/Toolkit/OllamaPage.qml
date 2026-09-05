/*
* Audacity: A Digital Audio Editor
*
* OllamaPage
*
* The local Ollama suite manager: health and version, installed models,
* a small hand-curated offline catalog with hardware fit verdicts, a
* payment-free batch pull cart with progress, and bulk export of the
* installed-model list. Every request goes only to a loopback or private
* network host; there is no cloud model service anywhere in this page.
*
* API:
*     none (self contained; reads and writes its own persisted settings)
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Toolkit
import Audacity.Companion

Item {
    id: root

    property bool offline: !ollama.reachable
    property var pullRows: ({})

    OllamaClient {
        id: ollama

        onReachableChanged: {
            if (ollama.reachable) {
                ollama.refreshInstalledModels()
            }
        }
        onRequestFailed: function (what, reason) {
            recoveryCard.message = qsTrc("toolkit", "Could not %1: %2").arg(what).arg(reason)
        }
        onPullProgress: function (modelTag, completedBytes, totalBytes, status) {
            var rows = root.pullRows
            rows[modelTag] = { completed: completedBytes, total: totalBytes, status: status, done: false }
            root.pullRows = Object.assign({}, rows)
        }
        onPullFinished: function (modelTag, success, error) {
            var rows = root.pullRows
            rows[modelTag] = { completed: 0, total: 0, status: success ? qsTrc("toolkit", "Done") : error, done: true }
            root.pullRows = Object.assign({}, rows)
            cart.removeModel(modelTag)
            if (success) {
                ollama.refreshInstalledModels()
            }
        }
    }

    HardwareFitService {
        id: fitService
    }

    PullCartModel {
        id: cart
    }

    property var filteredInstalled: {
        var list = ollama.installedModels
        var query = searchBar.searchText
        if (!query || query.length === 0) {
            return list
        }
        var pattern = null
        try {
            pattern = new RegExp(query, "i")
        } catch (e) {
            pattern = null
        }
        return list.filter(function (m) {
            var name = m.name !== undefined ? m.name : (m.model !== undefined ? m.model : "")
            return pattern ? pattern.test(name) : name.toLowerCase().indexOf(query.toLowerCase()) >= 0
        })
    }

    property var filteredCatalog: {
        var list = ollama.wellKnownCatalog()
        var query = searchBar.searchText
        if (!query || query.length === 0) {
            return list
        }
        var pattern = null
        try {
            pattern = new RegExp(query, "i")
        } catch (e) {
            pattern = null
        }
        return list.filter(function (m) {
            return pattern ? pattern.test(m.tag) : m.tag.toLowerCase().indexOf(query.toLowerCase()) >= 0
        })
    }

    ExportSheet {
        id: exportSheet

        anchors.fill: parent
        z: 100

        onExportSucceeded: function (filePath) {
            recoveryCard.visible = false
        }
        onExportFailed: function (filePath) {
            recoveryCard.message = qsTrc("toolkit", "Could not write the export to %1.").arg(filePath)
        }
    }

    Component.onCompleted: ollama.refreshHealth()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            StyledTextLabel {
                text: qsTrc("toolkit", "Local model manager")
                font: M3.typography.headlineSmall
            }

            Item { Layout.fillWidth: true }

            StyledTextLabel {
                text: ollama.reachable
                      ? qsTrc("toolkit", "Connected, version %1").arg(ollama.version)
                      : qsTrc("toolkit", "Not connected")
                color: ollama.reachable ? M3.color.primary : M3.color.error
            }

            M3Button {
                text: qsTrc("toolkit", "Refresh")
                variant: "outlined"
                onClicked: {
                    ollama.refreshHealth()
                    ollama.refreshInstalledModels()
                }
            }
        }

        RecoveryCard {
            id: recoveryCard

            Layout.fillWidth: true
            visible: root.offline
            message: qsTrc("toolkit",
                            "The local model runtime is not reachable at %1. Install it and start it, then retry.").arg(ollama.host)
            logsFolderPath: ""

            onRetryRequested: ollama.refreshHealth()
        }

        M3SearchBar {
            id: searchBar

            Layout.fillWidth: true
            placeholder: qsTrc("toolkit", "Search installed and catalog models")
            showRegexBuilder: true
            objectName: "OllamaModelSearch"
            onRegexBuilderRequested: regexSheet.open()
        }

        RegexBuilderSheet {
            id: regexSheet

            anchors.fill: parent
            storeName: "toolkit-ollama"
            fieldLabel: qsTrc("toolkit", "Model search")
            onPatternAccepted: function (pattern) {
                searchBar.searchText = pattern
            }
        }

        StyledTextLabel {
            text: qsTrc("toolkit", "Installed models")
            font: M3.typography.titleMedium
        }

        BulkSelectionController {
            id: bulkBar

            Layout.fillWidth: true
            totalCount: root.filteredInstalled.length
            pageStart: 0
            pageEnd: Math.max(0, root.filteredInstalled.length - 1)
            destructiveActionLabel: qsTrc("toolkit", "Remove selected")

            onActionRequested: function (actionId, indexes) {
                if (actionId === "export") {
                    var rows = []
                    for (var i = 0; i < indexes.length; ++i) {
                        var idx = indexes[i]
                        if (idx < root.filteredInstalled.length) {
                            rows.push(root.filteredInstalled[idx])
                        }
                    }
                    exportSheet.rows = rows
                    exportSheet.open()
                }
                // "destructive" bulk removal of an installed model would
                // call an Ollama delete endpoint here; not implemented in
                // this pass, so the button stays inert beyond reporting
                // the selection.
            }
        }

        ListView {
            id: installedList

            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(200, installedList.contentHeight)
            clip: true

            model: root.filteredInstalled

            delegate: M3ListItem {
                required property var modelData
                required property int index

                width: installedList.width
                headline: modelData.name !== undefined ? modelData.name : (modelData.model !== undefined ? modelData.model : "")
                supportingText: fitService.verdictFor(modelData.size !== undefined ? modelData.size : 0)
                trailingText: bulkBar.destructiveActionLabel
            }
        }

        StyledTextLabel {
            visible: root.filteredInstalled.length === 0
            text: ollama.reachable
                  ? qsTrc("toolkit", "No installed models match this search.")
                  : qsTrc("toolkit", "No installed models were found yet. Connect to the local runtime to list them.")
            color: M3.color.onSurfaceVariant
        }

        StyledTextLabel {
            text: qsTrc("toolkit", "Catalog (small offline list, not the exhaustive live catalog)")
            font: M3.typography.titleMedium
        }

        ListView {
            id: catalogList

            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(220, catalogList.contentHeight)
            clip: true

            model: root.filteredCatalog

            delegate: M3ListItem {
                required property var modelData

                width: catalogList.width
                headline: modelData.tag
                supportingText: fitService.verdictFor(modelData.approxBytes) + " · "
                                + qsTrc("toolkit", "~%1 GiB").arg((modelData.approxBytes / (1024 * 1024 * 1024)).toFixed(1))
                trailingText: cart.contains(modelData.tag) ? qsTrc("toolkit", "In cart") : qsTrc("toolkit", "Add")

                onClicked: {
                    if (cart.contains(modelData.tag)) {
                        cart.removeModel(modelData.tag)
                    } else {
                        cart.addModel(modelData.tag, modelData.approxBytes)
                    }
                }
            }
        }

        StyledTextLabel {
            text: qsTrc("toolkit", "Cart: %1 model(s) queued for pull").arg(cart.items.length)
            font: M3.typography.titleMedium
        }

        StyledTextLabel {
            visible: cart.items.length === 0
            text: qsTrc("toolkit",
                         "Add a catalog model to schedule a local download. Nothing here is bought or sold.")
            color: M3.color.onSurfaceVariant
        }

        Repeater {
            model: cart.items

            delegate: RowLayout {
                id: cartRow

                required property var modelData

                readonly property var progress: root.pullRows[cartRow.modelData.tag]

                Layout.fillWidth: true
                spacing: 8

                StyledTextLabel {
                    Layout.fillWidth: true
                    text: cartRow.modelData.tag
                }

                StyledTextLabel {
                    text: cartRow.progress
                          ? (cartRow.progress.done ? cartRow.progress.status
                                                    : qsTrc("toolkit", "%1 / %2 bytes").arg(cartRow.progress.completed).arg(cartRow.progress.total))
                          : qsTrc("toolkit", "Queued")
                    color: M3.color.onSurfaceVariant
                }

                M3Button {
                    text: qsTrc("toolkit", "Pull now")
                    variant: "outlined"
                    enabled: ollama.reachable
                    onClicked: ollama.pullModel(cartRow.modelData.tag)
                }

                M3Button {
                    text: qsTrc("toolkit", "Cancel")
                    variant: "text"
                    onClicked: {
                        ollama.cancelPull(cartRow.modelData.tag)
                        cart.removeModel(cartRow.modelData.tag)
                    }
                }
            }
        }
    }
}
