/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick 2.15
import Muse.Ui 1.0
import Muse.UiComponents

import Audacity.Export 1.0
import Audacity.M3

StyledTableView {
    id: root

    property int channelCount: 1

    ChannelMappingTableViewModel {
        id: tableViewModel
    }

    model: tableViewModel
    headerCapitalization: Font.MixedCase
    horizontalHeaderNavigationEnabled: false
    showVerticalHeader: true

    sourceComponentCallback: function (type) {
        switch (type) {
        case ChannelMappingTableViewCellType.Mapping:
            return mappingCellComp
        }
        return null
    }

    Component.onCompleted: {
        tableViewModel.setChannelCount(root.channelCount)
        tableViewModel.load(false /* reload */)
    }

    onChannelCountChanged: {
        tableViewModel.setChannelCount(root.channelCount)
        tableViewModel.load(true /* reload */)
    }

    function commitChanges() {
        tableViewModel.commitChanges()
    }

    Component {
        id: mappingCellComp

        M3Switch {
            id: cb

            checked: Boolean(cb.val)

            property var itemData
            property var val
            property int row
            property int column

            property NavigationPanel navigationPanel
            property int navigationRow
            property int navigationColumnStart
            property string accessibleName: val

            signal changed(var value)
            signal editingFinished

            navigation.panel: navigationPanel
            navigation.enabled: root.currentEditedCell === item
            navigation.row: navigationRow
            navigation.column: navigationColumnStart

            onToggled: {
                tableViewModel.handleEdit(row, column)
                cb.checked = Qt.binding(function () {
                    return Boolean(cb.val)
                })
            }
        }
    }
}
