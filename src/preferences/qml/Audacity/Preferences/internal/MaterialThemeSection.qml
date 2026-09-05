/*
* Audacity: A Digital Audio Editor
*
* The Material 3 half of the appearance page: the seed colour the whole
* palette is generated from, the scheme variant, the density level and the
* reduced motion override.
*/
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.UiComponents

import Audacity.M3

BaseSection {
    id: root

    title: qsTrc("appshell/preferences", "Material theme")

    property int density: 0
    property bool reducedMotion: false

    signal densityChangeRequested(int level)
    signal reducedMotionChangeRequested(bool enabled)

    Row {
        width: parent.width
        spacing: 24

        Column {
            spacing: 8

            StyledTextLabel {
                text: qsTrc("appshell/preferences", "Seed colour")
                font: M3.typography.bodyMedium
                color: M3.color.onSurfaceVariant
            }

            M3ColorPicker {
                id: seedPicker

                width: 320

                selection: M3.seedColor.toString()
                allowRainbow: false
                contrastBackground: M3.color.surface

                navigationPanel: root.navigation

                onAccepted: {
                    if (seedPicker.selection !== "rainbow") {
                        M3.seedColor = seedPicker.currentColor
                    }
                }
            }
        }

        Column {
            spacing: 16

            Column {
                spacing: 8

                StyledTextLabel {
                    text: qsTrc("appshell/preferences", "Scheme variant")
                    font: M3.typography.bodyMedium
                    color: M3.color.onSurfaceVariant
                }

                M3Dropdown {
                    id: variantDropdown

                    width: root.columnWidth

                    model: M3.variants
                    currentIndex: M3.variants.indexOf(M3.variant)

                    navigation.name: "M3SchemeVariant"
                    navigation.panel: root.navigation
                    navigation.row: 1

                    onActivated: function (index, value) {
                        M3.variant = M3.variants[index]
                    }
                }
            }

            Column {
                spacing: 8

                StyledTextLabel {
                    text: qsTrc("appshell/preferences", "Density")
                    font: M3.typography.bodyMedium
                    color: M3.color.onSurfaceVariant
                }

                M3SegmentedButton {
                    id: densitySelector

                    model: [qsTrc("appshell/preferences", "Comfortable"), qsTrc("appshell/preferences", "Cosy"), qsTrc("appshell/preferences", "Compact"), qsTrc("appshell/preferences", "Dense")]

                    currentIndex: -root.density

                    navigationPanel: root.navigation
                    navigationRowStart: 2

                    onActivated: function (index) {
                        root.densityChangeRequested(-index)
                    }
                }
            }

            M3Switch {
                id: reducedMotionSwitch

                text: qsTrc("appshell/preferences", "Reduce motion")
                checked: root.reducedMotion

                navigation.name: "M3ReducedMotion"
                navigation.panel: root.navigation
                navigation.row: 3

                onToggled: function (isOn) {
                    root.reducedMotionChangeRequested(isOn)
                    reducedMotionSwitch.checked = Qt.binding(function () {
                        return root.reducedMotion
                    })
                }
            }
        }
    }
}
