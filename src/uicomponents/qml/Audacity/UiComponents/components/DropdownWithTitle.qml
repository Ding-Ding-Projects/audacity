/*
* Audacity: A Digital Audio Editor
*/
import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

RowLayout {
    id: root

    property string title: ""
    property string current: ""
    property string toggleAccessibleName: title
    property string dropdownAccessibleName: title
    property var model: null

    property bool isOptionEnabled: false
    property bool allowOptionToggle: true

    property alias isOpened: dropdown.opened

    property NavigationControl navigation: allowOptionToggle ? optionCheckBox.navigation : dropdown.navigation
    property bool drawFocusBorderInsideRect: false

    signal isOptionEnableChangeRequested(var enable)
    signal handleMenuItem(var itemId)

    spacing: 8

    StyledTextLabel {
        Layout.alignment: Qt.AlignVCenter

        text: root.title
        font: M3.typography.bodyMedium
        color: root.enabled ? M3.color.onSurface : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
        visible: text !== ""
    }

    M3Checkbox {
        id: optionCheckBox

        Layout.alignment: Qt.AlignVCenter
        Layout.preferredWidth: touchTargetSize
        Layout.preferredHeight: touchTargetSize

        touchTargetSize: Math.min(32, root.height)

        checked: root.isOptionEnabled
        visible: root.allowOptionToggle
        accessibleName: root.toggleAccessibleName

        onClicked: {
            root.isOptionEnableChangeRequested(!root.isOptionEnabled)
        }
    }

    M3Dropdown {
        id: dropdown

        Layout.fillWidth: true
        Layout.fillHeight: true

        enabled: root.allowOptionToggle ? root.isOptionEnabled : true

        menuModel: root.model
        displayText: root.current
        accessibleName: root.dropdownAccessibleName

        fieldHeight: root.height
        menuNavigationPanel: optionCheckBox.navigation.panel

        navigation.panel: optionCheckBox.navigation.panel
        navigation.row: optionCheckBox.navigation.row
        navigation.column: optionCheckBox.navigation.column + 1

        onHandleMenuItem: function (itemId) {
            root.handleMenuItem(itemId)
        }
    }
}
