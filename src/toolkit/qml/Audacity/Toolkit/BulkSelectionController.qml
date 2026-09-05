/*
* Audacity: A Digital Audio Editor
*
* BulkSelectionController
*
* A reusable bulk-action bar for any list. Wraps the C++ BulkSelectionModel
* and shows the honestly-scoped selection count, a select-all versus
* select-all-matches choice, invert, and the caller-provided actions.
* Destructive actions must go through the host's own super confirmation
* surface before applying; this control only reports what was chosen.
*
* API:
*     totalCount, pageStart, pageEnd, destructiveActionLabel
*     actionRequested(string actionId, var selectedIndexes)
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Toolkit

RowLayout {
    id: root

    property int totalCount: 0
    property int pageStart: 0
    property int pageEnd: 0
    property string destructiveActionLabel: qsTrc("toolkit", "Delete")

    signal actionRequested(string actionId, var selectedIndexes)

    spacing: 8

    BulkSelectionModel {
        id: selection

        totalCount: root.totalCount
    }

    StyledTextLabel {
        text: selection.allMatchesSelected
              ? qsTrc("toolkit", "%1 selected (every match)").arg(selection.selectedCount)
              : qsTrc("toolkit", "%1 selected").arg(selection.selectedCount)
    }

    M3Button {
        text: qsTrc("toolkit", "Select page")
        variant: "text"
        onClicked: selection.selectAllOnPage(root.pageStart, root.pageEnd)
    }

    M3Button {
        text: qsTrc("toolkit", "Select all matches")
        variant: "text"
        onClicked: selection.selectAllMatches()
    }

    M3Button {
        text: qsTrc("toolkit", "Invert")
        variant: "text"
        onClicked: selection.invert()
    }

    M3Button {
        text: qsTrc("toolkit", "Clear")
        variant: "text"
        enabled: selection.selectedCount > 0
        onClicked: selection.clearSelection()
    }

    Item { Layout.fillWidth: true }

    M3Button {
        text: qsTrc("toolkit", "Export selected")
        variant: "outlined"
        enabled: selection.selectedCount > 0
        onClicked: root.actionRequested("export", selection.selectedIndexes())
    }

    M3Button {
        text: root.destructiveActionLabel
        variant: "outlined"
        enabled: selection.selectedCount > 0
        onClicked: root.actionRequested("destructive", selection.selectedIndexes())
    }
}
