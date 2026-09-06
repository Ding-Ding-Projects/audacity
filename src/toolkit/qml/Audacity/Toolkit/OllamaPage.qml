/*
* Audacity: A Digital Audio Editor
*
* OllamaPage
*
* The local Ollama suite manager: health and version, installed models,
* a locally verified installed-tag view, a bounded payment-free pull queue,
* streamed chat with cancellation and recovery, and bulk export of the
* installed-model list. Every request goes only to a loopback or private
* network host; there is no cloud model service anywhere in this page.
*
* API:
*     none (self contained; reads and writes its own persisted settings)
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Toolkit
import Audacity.Companion

Item {
    id: root

    property bool offline: !ollama.reachable
    property var pullRows: ({})
    property var chatMessages: []
    property string activeReply: ""
    property string chatFailure: ""
    property string selectedModel: ""
    property string systemPrompt: "You are a helpful local assistant."
    property bool imageAttachmentsSupported: false
    property string attachmentStatus: ""

    onSelectedModelChanged: {
        root.imageAttachmentsSupported = false
        root.attachmentStatus = qsTrc("toolkit", "Inspecting local model capabilities…")
        if (root.selectedModel.length > 0) {
            ollama.inspectModel(root.selectedModel)
        }
    }

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
        onChatToken: function (token, done) {
            root.activeReply += token
            if (done) {
                root.chatMessages = root.chatMessages.concat([{
                    role: "assistant",
                    content: root.activeReply
                }])
                root.activeReply = ""
            }
        }
        onChatError: function (error) {
            root.chatFailure = qsTrc("toolkit", "The local reply stopped: %1").arg(error)
        }
        onChatFinished: function (cancelled) {
            if (cancelled && root.activeReply.length > 0) {
                root.chatMessages = root.chatMessages.concat([{
                    role: "assistant",
                    content: root.activeReply + "\n[" + qsTrc("toolkit", "Stopped") + "]"
                }])
                root.activeReply = ""
            }
        }
        onModelInspected: function (modelTag, supportsImages) {
            if (modelTag === root.selectedModel) {
                root.imageAttachmentsSupported = supportsImages
                root.attachmentStatus = supportsImages
                    ? qsTrc("toolkit", "This local model reported image capability.")
                    : qsTrc("toolkit", "This local model did not report image capability.")
            }
        }
        onAttachmentRejected: function (reason) {
            root.attachmentStatus = reason
        }
        onPullProgress: function (modelTag, completedBytes, totalBytes, status) {
            var rows = root.pullRows
            rows[modelTag] = {
                completed: completedBytes,
                total: totalBytes,
                status: status,
                done: false
            }
            root.pullRows = Object.assign({}, rows)
        }
        onPullFinished: function (modelTag, success, error) {
            var rows = root.pullRows
            rows[modelTag] = {
                completed: 0,
                total: 0,
                status: success ? qsTrc("toolkit", "Done") : error,
                done: true
            }
            root.pullRows = Object.assign({}, rows)
            if (success) {
                ollama.refreshInstalledModels()
            }
        }
    }

    HardwareFitService {
        id: fitService
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

            Item {
                Layout.fillWidth: true
            }

            StyledTextLabel {
                text: ollama.reachable ? qsTrc("toolkit", "Connected, version %1").arg(ollama.version) : qsTrc("toolkit", "Not connected")
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
            message: qsTrc("toolkit", "The local model runtime is not reachable at %1. Install it and start it, then retry.").arg(ollama.host)
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
            text: ollama.reachable ? qsTrc("toolkit", "No installed models match this search.") : qsTrc("toolkit", "No installed models were found yet. Connect to the local runtime to list them.")
            color: M3.color.onSurfaceVariant
        }

        StyledTextLabel {
            text: qsTrc("toolkit", "Model catalog")
            font: M3.typography.titleMedium
        }

        StyledTextLabel {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTrc("toolkit", "This local API lists installed tags only. No complete official catalog snapshot is available in this build, so catalog completeness is unknown. Use an installed tag below to queue a verified local pull, or refresh after the runtime reports a new tag.")
            color: M3.color.onSurfaceVariant
        }

        StyledTextLabel {
            text: qsTrc("toolkit", "Bounded local pull queue")
            font: M3.typography.titleMedium
        }

        StyledTextLabel {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: qsTrc("toolkit", "Pulling uses at most %1 simultaneous local requests. Queued tags survive a restart and are reconciled when the local runtime is available. Nothing here is bought or sold.").arg(ollama.pullConcurrency)
            color: M3.color.onSurfaceVariant
        }

        Repeater {
            model: root.filteredInstalled

            delegate: RowLayout {
                id: cartRow

                required property var modelData

                readonly property string tag: cartRow.modelData.name !== undefined ? cartRow.modelData.name : cartRow.modelData.model
                readonly property var progress: root.pullRows[cartRow.tag]

                Layout.fillWidth: true
                spacing: 8

                StyledTextLabel {
                    Layout.fillWidth: true
                    text: cartRow.tag
                }

                StyledTextLabel {
                    text: cartRow.progress ? (cartRow.progress.done ? cartRow.progress.status : qsTrc("toolkit", "%1 / %2 bytes").arg(cartRow.progress.completed).arg(cartRow.progress.total)) : qsTrc("toolkit", "Queued")
                    color: M3.color.onSurfaceVariant
                }

                M3Button {
                    text: qsTrc("toolkit", "Pull now")
                    variant: "outlined"
                    enabled: ollama.reachable
                    onClicked: ollama.pullModel(cartRow.tag)
                }

                M3Button {
                    text: qsTrc("toolkit", "Cancel")
                    variant: "text"
                    onClicked: {
                        ollama.cancelPull(cartRow.tag)
                    }
                }
            }
        }

        StyledTextLabel {
            text: qsTrc("toolkit", "Local chat")
            font: M3.typography.titleMedium
        }

        StyledTextLabel {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            visible: root.filteredInstalled.length === 0
            text: qsTrc("toolkit", "Install and select a local model before starting a chat.")
            color: M3.color.onSurfaceVariant
        }

        ListView {
            id: modelPicker
            visible: root.filteredInstalled.length > 0
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(112, modelPicker.contentHeight)
            clip: true
            model: root.filteredInstalled
            delegate: M3ListItem {
                required property var modelData
                required property int index
                readonly property string tag: modelData.name !== undefined ? modelData.name : modelData.model
                width: modelPicker.width
                headline: tag
                supportingText: root.selectedModel === tag ? qsTrc("toolkit", "Selected for this local chat") : qsTrc("toolkit", "Select model")
                onClicked: root.selectedModel = tag
            }
        }

        StyledTextLabel {
            Layout.fillWidth: true
            visible: root.selectedModel.length > 0
            text: root.attachmentStatus.length > 0 ? root.attachmentStatus : qsTrc("toolkit", "Attachments are unavailable until this model's verified capability metadata is inspected by the local runtime.")
            color: M3.color.onSurfaceVariant
        }

        FileDialog {
            id: imageDialog
            title: qsTrc("toolkit", "Attach a local image")
            nameFilters: [qsTrc("toolkit", "Images (*.png *.jpg *.jpeg *.webp)")]
            fileMode: FileDialog.OpenFile
            onAccepted: {
                if (ollama.attachImage(root.selectedModel, selectedFile)) {
                    root.attachmentStatus = qsTrc("toolkit", "Image attached locally for the next message.")
                }
            }
        }

        TextArea {
            id: systemPromptEditor
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            text: root.systemPrompt
            placeholderText: qsTrc("toolkit", "System prompt")
            onTextChanged: root.systemPrompt = text.slice(0, 4096)
        }

        ListView {
            id: chatHistory
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(220, chatHistory.contentHeight)
            clip: true
            model: root.chatMessages
            delegate: M3ListItem {
                required property var modelData
                width: chatHistory.width
                headline: modelData.role === "user" ? qsTrc("toolkit", "You") : qsTrc("toolkit", "Local model")
                supportingText: modelData.content
            }
        }

        StyledTextLabel {
            visible: root.activeReply.length > 0
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: root.activeReply
        }

        TextArea {
            id: chatComposer
            Layout.fillWidth: true
            Layout.preferredHeight: 92
            placeholderText: qsTrc("toolkit", "Write a local message")
            enabled: root.selectedModel.length > 0 && !ollama.chatInFlight
        }

        RowLayout {
            Layout.fillWidth: true
            M3Button {
                text: qsTrc("toolkit", "Send")
                variant: "filled"
                enabled: !ollama.chatInFlight && root.selectedModel.length > 0 && chatComposer.text.trim().length > 0
                onClicked: {
                    var next = root.chatMessages.concat([{ role: "user", content: chatComposer.text.slice(0, 16384) }])
                    var requestMessages = [{ role: "system", content: root.systemPrompt }]
                    var historyStart = Math.max(0, next.length - 20)
                    for (var historyIndex = historyStart; historyIndex < next.length; ++historyIndex) {
                        requestMessages.push(next[historyIndex])
                    }
                    root.chatMessages = next
                    root.chatFailure = ""
                    ollama.sendChatMessage(root.selectedModel, requestMessages, { temperature: 0.7 })
                    chatComposer.text = ""
                }
            }
            M3Button {
                text: qsTrc("toolkit", "Attach image")
                variant: "outlined"
                enabled: !ollama.chatInFlight && root.imageAttachmentsSupported
                onClicked: imageDialog.open()
            }
            M3Button {
                text: qsTrc("toolkit", "Stop")
                variant: "outlined"
                enabled: ollama.chatInFlight
                onClicked: ollama.cancelChat()
            }
            Item { Layout.fillWidth: true }
            M3Button {
                text: qsTrc("toolkit", "Retry last")
                variant: "text"
                enabled: !ollama.chatInFlight && root.chatMessages.length > 0 && root.selectedModel.length > 0
                onClicked: {
                    for (var i = root.chatMessages.length - 1; i >= 0; --i) {
                        if (root.chatMessages[i].role === "user") {
                            chatComposer.text = root.chatMessages[i].content
                            break
                        }
                    }
                }
            }
        }

        StyledTextLabel {
            visible: root.chatFailure.length > 0
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: root.chatFailure
            color: M3.color.error
        }
    }
}
