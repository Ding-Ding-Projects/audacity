/*
* Audacity: A Digital Audio Editor
*/
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.ProjectScene
import Audacity.M3

import "internal"

StyledToolBarView {
    id: root

    navigationPanel.name: "UndoRedoToolBar"
    navigationPanel.accessible.name: qsTrc("projectscene", "Undo and redo")

    spacing: 2
    rowHeight: M3.density.apply(40)

    model: UndoRedoToolBarModel {}

    sourceComponentCallback: function (type) {
        switch (type) {
        case ToolBarItemType.ACTION:
            return controlComp
        }

        return null
    }

    Component {
        id: controlComp

        M3ToolBarItem {
            width: M3.density.apply(40)
            height: width

            navigation.panel: root.navigationPanel
        }
    }
}
