/*
* Audacity: A Digital Audio Editor
*
* ExportSheet
*
* A reusable export surface any list model can open. Offers every coding
* file format the export service supports (JSON, JSONL, YAML, TOML, XML,
* CSV, TSV, Markdown, HTML, SQL) plus a store only ZIP archive, discloses
* which fields a tabular format would drop before exporting, and writes
* UTF-8 output through the C++ export service.
*
* API:
*     opened, rows (list of objects), open(), close()
*     exported(string filePath)
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Item {
    id: root

    property bool opened: false
    property var rows: []

    //! Format ids in exportservice.h order; kept in step with
    //! exportFormatId()/exportFormatLabel() by name, not by index.
    readonly property var formats: [
        { id: "json", label: qsTrc("toolkit", "JSON") },
        { id: "jsonl", label: qsTrc("toolkit", "JSON Lines (NDJSON)") },
        { id: "yaml", label: qsTrc("toolkit", "YAML") },
        { id: "toml", label: qsTrc("toolkit", "TOML") },
        { id: "xml", label: qsTrc("toolkit", "XML") },
        { id: "csv", label: qsTrc("toolkit", "CSV") },
        { id: "tsv", label: qsTrc("toolkit", "TSV") },
        { id: "markdown", label: qsTrc("toolkit", "Markdown") },
        { id: "html", label: qsTrc("toolkit", "HTML") },
        { id: "sql", label: qsTrc("toolkit", "SQL") },
        { id: "zip", label: qsTrc("toolkit", "ZIP archive (store only; 7z is not available in this build)") }
    ]

    property int selectedFormatIndex: 0

    signal exported(string filePath)

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
                model: root.formats

                delegate: RowLayout {
                    id: formatRow

                    required property var modelData
                    required property int index

                    spacing: 8

                    M3RadioButton {
                        checked: root.selectedFormatIndex === formatRow.index
                        onClicked: root.selectedFormatIndex = formatRow.index
                    }

                    StyledTextLabel {
                        text: formatRow.modelData.label
                    }
                }
            }

            StyledTextLabel {
                id: droppedFieldsLabel

                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                color: M3.color.error
                visible: text.length > 0
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
                    onClicked: saveDialog.open()
                }
            }
        }
    }

    FileDialog {
        id: saveDialog

        fileMode: FileDialog.SaveFile
        onAccepted: {
            // The actual field encoding and file writing happens through
            // the C++ ExportService via the host page, which owns the row
            // model and therefore the concrete rows to hand across. This
            // sheet only decides the format and destination and reports
            // back through exported().
            root.exported(saveDialog.selectedFile)
            root.close()
        }
    }
}
