/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.AppShell
import Audacity.UiComponents

import Audacity.M3

Rectangle {
    id: root

    color: M3.color.surfaceContainer

    TableTestsModel {
        id: source

        Component.onCompleted: load()
    }

    TableSortFilterProxyModel {
        id: proxy

        sourceModel: source
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Text {
            color: M3.color.onSurface
            text: "Minimal AbstractTableViewModel impl with default TableSortFilterProxyModel for sorting."
            font: M3.typography.bodyLarge
        }

        Text {
            color: M3.color.onSurface
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: "Click a column header to toggle its sort direction.\nRepeated clicks on different headers build a multi-key stable sort.\nThe default compareCells() extracts each cell's muse::Val and compares it; acceptsRow() accepts every row."
            horizontalAlignment: Text.AlignLeft
        }

        RowLayout {
            spacing: 8

            M3Button {
                text: "Clear sort"
                onClicked: proxy.clearSort()
            }
        }

        StyledTableView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            model: proxy

            // TODO: https://github.com/audacity/audacity/issues/10852
            // onHorizontalHeaderClicked: function (column) {
            //     proxy.toggleColumnSort(column)
            // }
        }
    }
}
