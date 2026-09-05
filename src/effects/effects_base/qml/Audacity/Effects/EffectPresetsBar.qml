/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.Effects
import Audacity.M3

RowLayout {
    id: root

    required property var instanceId
    required property bool destructiveMode
    property alias realtimeEffectState: presetsBarModel.realtimeEffectState

    property int navigationOrder: 0
    property var navigationPanel: null

    property var parentWindow: null

    // Expose the presets bar model so parent can listen to its signals
    property alias presetsBarModel: presetsBarModel

    spacing: 4

    QtObject {
        id: prv

        property AbstractMenuModel activeMenuModel: null

        function openThreeDotMenu(button) {
            activeMenuModel = presetsBarModel.presetContextMenu()
            var pos = Qt.point(button.x, button.y + button.height)
            menuLoader.show(pos, activeMenuModel)
        }

        function save(button) {
            activeMenuModel = presetsBarModel.saveContextMenu()
            var pos = Qt.point(button.x, button.y + button.height)
            menuLoader.show(pos, activeMenuModel)
        }
    }

    Component.onCompleted: {
        Qt.callLater(presetsBarModel.load)
    }

    EffectPresetsBarModel {
        id: presetsBarModel
        instanceId: root.instanceId
        persistLastUsedPreset: root.destructiveMode
    }

    Connections {
        target: presetsBarModel

        function onPresetChanged() {
            presetSelector.currentIndex = presetsBarModel.presets.findIndex(preset => preset.id === presetsBarModel.preset)
        }

        function onPresetsChanged() {
            presetSelector.currentIndex = presetsBarModel.presets.findIndex(preset => preset.id === presetsBarModel.preset)
        }
    }

    ContextMenuLoader {
        id: menuLoader

        visible: false

        parentWindow: root.parentWindow

        onHandleMenuItem: function (itemId) {
            if (prv.activeMenuModel) {
                prv.activeMenuModel.handleMenuItem(itemId)
            }
        }
    }

    M3Dropdown {
        id: presetSelector

        navigation.panel: root.navigationPanel
        navigation.order: root.navigationOrder
        navigation.name: "preset dropdown"
        navigation.accessible.name: qsTrc("effects", "Select preset")

        Layout.fillWidth: true

        textRole: "name"
        valueRole: "id"

        enabled: presetsBarModel.presetsDropdownEnabled

        model: presetsBarModel.presets

        onActivated: function (index, value) {
            presetsBarModel.preset = value
            currentIndex = presetsBarModel.presets.findIndex(preset => preset.id === value)
        }
    }

    M3Button {
        id: saveBtn
        variant: "tonal"

        navigation.panel: root.navigationPanel
        navigation.order: presetSelector.navigation.order + 1
        navigation.name: "save preset btn"
        //: Tooltip of a button in the effect presets bar
        toolTipTitle: qsTrc("effects", "Save preset")

        Layout.alignment: Qt.AlignVCenter
        icon: IconCode.SAVE

        onClicked: {
            prv.save(saveBtn)
        }
    }

    M3Button {
        id: resetBtn
        variant: "tonal"

        navigation.panel: root.navigationPanel
        navigation.order: saveBtn.navigation.order + 1
        navigation.name: "reset preset btn"
        //: Tooltip of a button in the effect presets bar
        toolTipTitle: qsTrc("effects", "Reset preset")

        Layout.alignment: Qt.AlignVCenter

        icon: IconCode.UNDO
        enabled: presetsBarModel.canResetPreset

        onClicked: {
            presetsBarModel.resetPreset()
        }
    }

    M3Button {
        id: deleteBtn
        variant: "tonal"

        navigation.panel: root.navigationPanel
        navigation.order: resetBtn.navigation.order + 1
        navigation.name: "delete preset btn"
        //: Tooltip of a button in the effect presets bar
        toolTipTitle: qsTrc("effects", "Delete preset")

        Layout.alignment: Qt.AlignVCenter

        icon: IconCode.DELETE_TANK
        enabled: presetsBarModel.canDeletePreset

        onClicked: {
            presetsBarModel.deletePreset()
        }
    }

    M3Button {
        id: manageButton
        variant: "tonal"

        navigation.panel: root.navigationPanel
        navigation.order: deleteBtn.navigation.order + 1
        navigation.name: "manage preset btn"
        //: Tooltip of a button in the effect presets bar
        toolTipTitle: qsTrc("effects", "Preset options")

        Layout.alignment: Qt.AlignVCenter

        icon: IconCode.MENU_THREE_DOTS

        onClicked: {
            prv.openThreeDotMenu(manageButton)
        }
    }
}
