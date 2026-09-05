/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * The Material 3 seed colour picker. The preset swatches sit in a row and an
 * optional "Custom" swatch opens the full M3ColorPicker.
 */
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Item {
    id: root

    property var colors: []
    property int currentColorIndex: 0

    // Shows the trailing swatch that opens the Material 3 colour picker so the
    // seed colour can be chosen freely.
    property bool showCustom: false

    property NavigationPanel navigationPanel: NavigationPanel {
        name: "AccentColorsList"
        enabled: root.enabled && root.visible
        direction: NavigationPanel.Horizontal

        onNavigationEvent: function (event) {
            if (event.type === NavigationEvent.AboutActive) {
                event.setData("controlIndex", [navigationRow, navigationColumnStart + root.currentColorIndex]);
            }
        }
    }

    property int navigationRow: -1
    property int navigationColumnStart: 0
    readonly property int count: root.colors ? root.colors.length : 0
    property int navigationColumnEnd: navigationColumnStart + root.count

    property real sampleSize: 20
    readonly property real totalSampleSize: sampleSize + 10

    property real spacing: 6

    signal accentColorChangeRequested(var newColorIndex)

    // Emitted with the colour chosen in the picker, as a hexadecimal string.
    signal seedColorChangeRequested(string seedColor)

    implicitWidth: root.count * root.totalSampleSize + Math.max(0, root.count - 1) * root.spacing + (root.showCustom ? customButton.implicitWidth + root.spacing : 0)
    implicitHeight: root.totalSampleSize

    // The swatches stay on one line and scroll sideways so that the row never
    // runs past its host.
    Flickable {
        id: swatchArea

        anchors.left: parent.left
        anchors.right: root.showCustom ? customButton.left : parent.right
        anchors.rightMargin: root.showCustom ? root.spacing : 0
        anchors.verticalCenter: parent.verticalCenter

        height: root.totalSampleSize
        contentWidth: swatchRow.width
        contentHeight: height
        clip: true
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds

        Row {
            id: swatchRow

            height: root.totalSampleSize

            spacing: root.spacing

            Repeater {
                model: root.colors

                delegate: Item {
                    id: swatch

                    required property int index
                    required property var modelData

                    readonly property bool selected: root.currentColorIndex === swatch.index

                    width: root.totalSampleSize
                    height: root.totalSampleSize

                    NavigationControl {
                        id: navCtrl

                        name: "AccentColorButton"
                        panel: root.navigationPanel
                        row: root.navigationRow
                        column: root.navigationColumnStart + swatch.index
                        enabled: root.enabled && root.visible

                        accessible.role: MUAccessible.RadioButton
                        accessible.name: Utils.accessibleColorDescription(swatch.modelData)
                        accessible.checked: swatch.selected
                        accessible.visualItem: ring

                        onTriggered: {
                            root.accentColorChangeRequested(swatch.index);
                        }
                    }

                    Rectangle {
                        id: ring

                        anchors.fill: parent

                        color: "transparent"
                        radius: width / 2
                        border.width: swatch.selected ? 2 : 0
                        border.color: M3.color.primary

                        Behavior on border.width {
                            NumberAnimation {
                                duration: M3.motion.short3
                                easing: M3.motion.standard
                            }
                        }
                    }

                    M3StateLayer {
                        anchors.fill: parent
                        radius: width / 2
                        color: M3.color.onSurface
                        hovered: mouseArea.containsMouse
                        pressed: mouseArea.containsPress
                        focused: navCtrl.highlight
                    }

                    M3FocusRing {
                        anchors.fill: parent
                        shapeRadius: width / 2
                        visible: navCtrl.highlight
                    }

                    Rectangle {
                        anchors.centerIn: parent

                        width: root.sampleSize
                        height: width
                        radius: width / 2

                        border.color: M3.color.outlineVariant
                        border.width: 1

                        //! NOTE The swatch shows the seed colour itself, so it is
                        //! data rather than a Material 3 role.
                        color: swatch.modelData
                    }

                    MouseArea {
                        id: mouseArea

                        anchors.fill: parent
                        hoverEnabled: true

                        onClicked: {
                            root.accentColorChangeRequested(swatch.index);
                        }
                    }
                }
            }
        }
    }

    M3Button {
        id: customButton

        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter

        height: root.totalSampleSize

        visible: root.showCustom

        variant: "text"
        text: qsTrc("appshell/gettingstarted", "Custom")

        navigation.panel: root.navigationPanel
        navigation.row: root.navigationRow
        navigation.column: root.navigationColumnEnd

        onClicked: {
            colorPickerPopup.open();
        }
    }

    StyledPopupView {
        id: colorPickerPopup

        contentWidth: 320
        contentHeight: 420

        M3ColorPicker {
            id: colorPicker

            anchors.fill: parent

            allowRainbow: false
            selection: M3.seedColor.toString()

            navigationPanel: root.navigationPanel

            onAccepted: {
                M3.seedColor = colorPicker.selection;
                root.seedColorChangeRequested(colorPicker.selection);
                colorPickerPopup.close();
            }
        }
    }
}
