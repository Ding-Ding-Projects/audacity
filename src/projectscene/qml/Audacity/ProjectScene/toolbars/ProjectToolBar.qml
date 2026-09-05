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

    property alias isCompactMode: toolBarModel.isCompactMode

    navigationPanel.name: "ProjectToolBar"
    navigationPanel.accessible.name: qsTrc("projectscene", "Audio setup and sharing")

    spacing: 2
    rowHeight: M3.density.apply(40)

    sourceComponentCallback: function (type) {
        switch (type) {
        case ToolBarItemType.ACTION:
            return controlComp
        case ToolBarItemType.SEPARATOR:
            return separatorComp
        }

        return null
    }

    Component {
        id: controlComp

        M3ToolBarItem {
            navigation.panel: root.navigationPanel
        }
    }

    Component {
        id: separatorComp

        M3Divider {
            property var itemData: null

            width: 1
            height: root.separatorHeight
            orientation: Qt.Vertical
        }
    }

    model: ProjectToolBarModel {
        id: toolBarModel

        readonly property int bottomMargin: 10

        onOpenAudioSetupContextMenu: {
            audioSetupContextMenuLoader.show(Qt.point(root.width / 3, root.rowHeight + bottomMargin), audioSetupContextMenuModel.items)
        }

        onOpenGetEffectsDialog: {
            api.launcher.open("audacity://projectscene/geteffects")
        }
    }

    AudioSetupContextMenuModel {
        id: audioSetupContextMenuModel
    }

    ContextMenuLoader {
        id: audioSetupContextMenuLoader

        onHandleMenuItem: function (itemId) {
            audioSetupContextMenuModel.handleMenuItem(itemId)
        }
    }

    Component.onCompleted: {
        audioSetupContextMenuModel.load()
    }
}
