/*
* Audacity: A Digital Audio Editor
*
* CommandPaletteRow
*
* One row of the command palette. An action row shows its title, its section
* and its shortcut. A settings row shows the same, and puts a live control
* beside it: a switch, a slider, a dropdown or a colour swatch, bound to the
* same muse setting the originating preferences surface writes, so a change
* made here is the same change made there.
*
* API:
*     model (the palette model), row, rowType, title, subtitle, section,
*     shortcut, controlType, settingKey, settingValue, options, minimum,
*     maximum, step, selected, activated()
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property var paletteModel: null
    property int row: -1

    property string rowType: ""
    property string title: ""
    property string subtitle: ""
    property string section: ""
    property string shortcut: ""
    property bool rowEnabled: true
    property string controlType: ""
    property string settingKey: ""
    property var settingValue: undefined
    property var options: []
    property var optionLabels: []
    property real minimum: 0
    property real maximum: 1
    property real step: 1
    property bool selected: false

    property NavigationPanel navigationPanel: null

    signal activated

    // A row carries a live control only when it has a setting key behind it.
    // Everything else teleports to the surface that owns it.
    readonly property bool hasLiveControl: root.settingKey !== "" && (root.controlType === "switch" || root.controlType === "slider" || root.controlType === "dropdown" || root.controlType === "color")

    implicitHeight: Math.max(M3.density.apply(64), layout.implicitHeight + 16)

    function commit(value) {
        if (root.paletteModel !== null && root.settingKey !== "") {
            root.paletteModel.setSettingValue(root.settingKey, value)
        }
    }

    Rectangle {
        id: background

        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        radius: M3.shape.large
        color: root.selected ? M3.color.secondaryContainer : "transparent"

        M3StateLayer {
            anchors.fill: parent
            radius: background.radius
            color: M3.color.onSurface
            active: root.rowEnabled
            hovered: hoverArea.containsMouse
        }
    }

    MouseArea {
        id: hoverArea

        anchors.fill: background
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton
        onClicked: root.activated()
    }

    RowLayout {
        id: layout

        anchors.fill: background
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 12

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                elide: Text.ElideRight
                text: root.title
                font: M3.typography.bodyLarge
                color: root.rowEnabled ? M3.color.onSurface : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
            }

            StyledTextLabel {
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignLeft
                elide: Text.ElideRight
                visible: text !== ""
                text: root.subtitle !== "" ? root.section + " · " + root.subtitle : root.section
                font: M3.typography.bodySmall
                color: M3.color.onSurfaceVariant
            }
        }

        // The live control, when the row has one.
        Loader {
            id: controlLoader

            Layout.alignment: Qt.AlignVCenter
            visible: root.hasLiveControl
            active: root.hasLiveControl

            sourceComponent: {
                if (!root.hasLiveControl) {
                    return null
                }
                switch (root.controlType) {
                case "switch":
                    return switchComponent
                case "slider":
                    return sliderComponent
                case "dropdown":
                    return dropdownComponent
                case "color":
                    return colorComponent
                default:
                    return null
                }
            }
        }

        StyledTextLabel {
            Layout.alignment: Qt.AlignVCenter
            visible: root.shortcut !== ""
            text: root.shortcut
            font: M3.typography.labelMedium
            color: M3.color.onSurfaceVariant
        }

        StyledIconLabel {
            Layout.alignment: Qt.AlignVCenter
            visible: root.rowType === "setting" || root.rowType === "page" || root.rowType === "doc"
            iconCode: IconCode.ARROW_RIGHT
            color: M3.color.onSurfaceVariant
        }
    }

    Component {
        id: switchComponent

        M3Switch {
            checked: root.settingValue === true || root.settingValue === "true"
            accessibleName: root.title
            navigation.panel: root.navigationPanel
            onToggled: function (isChecked) {
                root.commit(isChecked)
            }
        }
    }

    Component {
        id: sliderComponent

        Row {
            spacing: 8

            M3Slider {
                id: slider

                width: 160
                anchors.verticalCenter: parent.verticalCenter

                from: root.minimum
                to: root.maximum
                stepSize: root.step
                value: Number(root.settingValue)
                accessibleName: root.title
                navigation.panel: root.navigationPanel

                onMoved: root.commit(slider.value)
            }

            StyledTextLabel {
                anchors.verticalCenter: parent.verticalCenter
                text: String(Math.round(Number(root.settingValue)))
                font: M3.typography.labelLarge
                color: M3.color.onSurfaceVariant
            }
        }
    }

    Component {
        id: dropdownComponent

        M3Dropdown {
            id: dropdown

            width: 200
            model: root.optionLabels.length === root.options.length && root.options.length > 0 ? root.options.map(function (value, index) {
                return {
                    "text": root.optionLabels[index],
                    "value": value
                }
            }) : root.options.map(function (value) {
                return {
                    "text": String(value),
                    "value": value
                }
            })

            currentIndex: {
                for (var i = 0; i < root.options.length; ++i) {
                    if (String(root.options[i]) === String(root.settingValue)) {
                        return i
                    }
                }
                return -1
            }

            placeholder: String(root.settingValue !== undefined ? root.settingValue : "")
            accessibleName: root.title

            onActivated: function (index, value) {
                root.commit(value)
            }
        }
    }

    Component {
        id: colorComponent

        Rectangle {
            width: 40
            height: 28
            radius: M3.shape.small
            color: root.settingValue !== undefined && String(root.settingValue) !== "" ? String(root.settingValue) : M3.color.primary
            border.width: 1
            border.color: M3.color.outlineVariant

            MouseArea {
                anchors.fill: parent
                onClicked: pickerLoader.active = true
            }

            Loader {
                id: pickerLoader

                active: false

                sourceComponent: M3ColorPicker {
                    selection: String(root.settingValue)
                    onAccepted: {
                        root.commit(selection)
                        pickerLoader.active = false
                    }
                }
            }
        }
    }
}
