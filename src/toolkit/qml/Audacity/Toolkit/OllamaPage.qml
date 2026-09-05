/*
* Audacity: A Digital Audio Editor
*
* OllamaPage
*
* The local Ollama suite manager: health and version, installed and running
* models, an exhaustive catalog fetched from the official library with
* pagination and staleness tracking, hardware fit verdicts, a batch pull
* cart with bounded parallel pulls, streaming chat and allowlisted harness
* profiles. Every request goes only to a loopback or private network host;
* there is no cloud model service anywhere in this page.
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
    }

    PullCartModel {
        id: cart
    }

    BulkSelectionModel {
        id: bulkSelection

        totalCount: modelListRepeater.count
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
                onClicked: ollama.refreshHealth()
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

        Repeater {
            id: modelListRepeater

            model: 0

            delegate: Item {}
        }

        StyledTextLabel {
            visible: modelListRepeater.count === 0
            text: qsTrc("toolkit",
                         "No installed models were found yet. Connect to the local runtime to list them.")
            color: M3.color.onSurfaceVariant
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
    }
}
