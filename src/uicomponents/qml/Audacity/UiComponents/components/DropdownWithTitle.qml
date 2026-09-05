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

    property alias isOpened: menuLoader.isMenuOpened

    property NavigationControl navigation: allowOptionToggle ? optionCheckBox.navigation : navCtrl
    property bool drawFocusBorderInsideRect: false

    signal isOptionEnableChangeRequested(var enable)
    signal handleMenuItem(var itemId)

    spacing: 6

    StyledTextLabel {
        Layout.alignment: Qt.AlignVCenter

        text: root.title
        font: M3.typography.bodyMedium
        color: M3.color.onSurface
        visible: text !== ""
    }

    M3Switch {
        id: optionCheckBox

        Layout.alignment: Qt.AlignVCenter

        checked: root.isOptionEnabled
        visible: root.allowOptionToggle
        navigation.accessible.name: root.toggleAccessibleName

        onToggled: function (isOn) {
            root.isOptionEnableChangeRequested(isOn);
            optionCheckBox.checked = Qt.binding(function () {
                return root.isOptionEnabled;
            });
        }
    }

    Item {
        id: dropdown

        Layout.fillWidth: true
        Layout.fillHeight: true

        enabled: root.allowOptionToggle ? root.isOptionEnabled : true

        function openMenu() {
            menuLoader.toggleOpened(root.model);
        }

        NavigationControl {
            id: navCtrl

            name: "DropdownWithTitleItem"
            enabled: dropdown.enabled && dropdown.visible
            accessible.role: MUAccessible.Button
            accessible.name: root.dropdownAccessibleName ? root.dropdownAccessibleName + ": " + root.current : root.current

            panel: optionCheckBox.navigation.panel
            row: optionCheckBox.navigation.row
            column: optionCheckBox.navigation.column + 1

            onActiveChanged: {
                if (!dropdown.activeFocus) {
                    dropdown.forceActiveFocus();
                }
            }

            onTriggered: {
                dropdown.openMenu();
            }
        }

        Rectangle {
            id: backgroundItem
            anchors.fill: parent

            NavigationFocusBorder {
                navigationCtrl: navCtrl
                drawOutsideParent: !root.drawFocusBorderInsideRect
            }

            color: M3.color.surfaceContainerHighest
            border.color: M3.color.outline
            border.width: 1
            radius: M3.shape.extraSmall
        }

        StyledTextLabel {
            id: labelItem

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.right: dropIconItem.left
            anchors.verticalCenter: parent.verticalCenter

            text: root.current
            font: M3.typography.bodyMedium
            color: M3.color.onSurface
            horizontalAlignment: Text.AlignLeft
            wrapMode: Text.Wrap
            maximumLineCount: 1
        }

        MouseArea {
            id: mouseAreaItem
            anchors.fill: parent
            hoverEnabled: dropdown.enabled

            onClicked: {
                dropdown.openMenu();
            }

            onPressed: {
                ui.tooltip.hide(dropdown, true);
            }

            onContainsMouseChanged: {
                if (!labelItem.truncated || menuLoader.isMenuOpened) {
                    return;
                }

                if (mouseAreaItem.containsMouse) {
                    ui.tooltip.show(dropdown, labelItem.text);
                } else {
                    ui.tooltip.hide(dropdown);
                }
            }
        }

        StyledIconLabel {
            id: dropIconItem
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 8

            iconCode: IconCode.SMALL_ARROW_DOWN
        }

        states: [
            State {
                name: "HOVERED"
                when: mouseAreaItem.containsMouse && !mouseAreaItem.pressed
                PropertyChanges {
                    target: backgroundItem
                    border.color: Utils.colorWithAlpha(M3.color.primary, 0.6)
                }
            },
            State {
                name: "OPENED"
                when: menuLoader.isMenuOpened
                PropertyChanges {
                    target: backgroundItem
                    border.color: M3.color.primary
                }
            }
        ]

        StyledMenuLoader {
            id: menuLoader

            onHandleMenuItem: function (itemId) {
                root.handleMenuItem(itemId);
            }
        }
    }
}
