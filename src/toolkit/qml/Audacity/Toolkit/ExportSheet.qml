/*
* Audacity: A Digital Audio Editor
*
* ExportSheet
*
* A reusable export surface any list model can open. Offers every coding
* file format the export service supports (JSON, JSONL, YAML, TOML, XML,
* CSV, TSV, Markdown, HTML, SQL) plus a store only ZIP archive, discloses
* which fields the chosen format would drop before the destination is even
* picked, and writes UTF-8 output through the C++ export service.
*
* API:
*     opened, rows (list of objects), open(), close()
*     exportSucceeded(string filePath), exportFailed(string filePath)
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Toolkit

Item {
    id: root

    property bool opened: false
    property var rows: []
    property int selectedFormatIndex: 0

    signal exportSucceeded(string filePath)
    signal exportFailed(string filePath)

    ExportServiceWrapper {
        id: exportService
    }

    readonly property var formatIds: exportService.formatIds()

    readonly property string currentFormatId: root.formatIds.length > 0 ? root.formatIds[Math.min(root.selectedFormatIndex, root.formatIds.length - 1)] : ""

    readonly property var currentDroppedFields: root.currentFormatId.length > 0 ? exportService.droppedFields(root.currentFormatId, root.rows) : []

    function open() {
        root.opened = true
    }

    function close() {
        root.opened = false
    }

    visible: root.opened

    M3Card {
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 520)
        elevation: 3

        ColumnLayout {
            width: parent.width
            spacing: 12

            StyledTextLabel {
                text: qsTrc("toolkit", "Export")
                font: M3.typography.titleLarge
            }

            StyledTextLabel {
                text: qsTrc("toolkit", "%1 row(s) will be exported.").arg(root.rows.length)
            }

            Repeater {
                model: root.formatIds

                delegate: RowLayout {
                    id: formatRow

                    required property string modelData
                    required property int index

                    spacing: 8

                    M3RadioButton {
                        checked: root.selectedFormatIndex === formatRow.index
                        onClicked: root.selectedFormatIndex = formatRow.index
                    }

                    StyledTextLabel {
                        text: exportService.formatLabel(formatRow.modelData)
                    }
                }
            }

            StyledTextLabel {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: M3.color.error
                visible: root.currentDroppedFields.length > 0
                text: qsTrc("toolkit", "This format cannot carry every field. Dropped on export: %1.").arg(root.currentDroppedFields.join(", "))
            }

            RowLayout {
                spacing: 8

                M3Button {
                    text: qsTrc("toolkit", "Cancel")
                    variant: "text"
                    onClicked: root.close()
                }

                M3Button {
                    text: qsTrc("toolkit", "Choose destination and export")
                    variant: "filled"
                    enabled: root.rows.length > 0
                    onClicked: saveDialog.open()
                }
            }
        }
    }

    FileDialog {
        id: saveDialog

        fileMode: FileDialog.SaveFile
        onAccepted: {
            const path = saveDialog.selectedFile.toString().replace(/^file:\/\//, "")
            const ok = exportService.exportRows(root.currentFormatId, root.rows, path)
            if (ok) {
                root.exportSucceeded(path)
                root.close()
            } else {
                root.exportFailed(path)
            }
        }
    }
}
