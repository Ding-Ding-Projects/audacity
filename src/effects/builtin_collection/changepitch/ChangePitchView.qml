/*
* Audacity: A Digital Audio Editor
*/
import QtQuick 2.15

import Muse.Ui 1.0
import Muse.UiComponents

import Audacity.Effects
import Audacity.BuiltinEffects
import Audacity.BuiltinEffectsCollection
import Audacity.M3

BuiltinEffectBase {
    id: root

    width: prv.desiredWidth - (2 * prv.spaceXL) // we need to remove the padding from the dialog desired width
    implicitHeight: mainColumn.height // see with EffectsViewerDialog.qml

    property string title: changePitch.effectTitle()
    property bool isApplyAllowed: true

    builtinEffectModel: ChangePitchViewModelFactory.createModel(root, root.instanceId)
    numNavigationPanels: 4
    property alias changePitch: root.builtinEffectModel
    property NavigationPanel pitchRowNavigationPanel: NavigationPanel {
        name: "ChangePitchPitchRow"
        enabled: root.enabled && root.visible
        direction: NavigationPanel.Horizontal
        section: root.dialogView ? root.dialogView.navigationSection : null
        order: 1
    }
    property NavigationPanel semitonesAndCentsNavigationPanel: NavigationPanel {
        name: "ChangePitchSemitonesAndCents"
        enabled: root.enabled && root.visible
        direction: NavigationPanel.Horizontal
        section: root.dialogView ? root.dialogView.navigationSection : null
        order: 2
    }
    property NavigationPanel frequencyNavigationPanel: NavigationPanel {
        name: "ChangePitchFrequency"
        enabled: root.enabled && root.visible
        direction: NavigationPanel.Horizontal
        section: root.dialogView ? root.dialogView.navigationSection : null
        order: 3
    }
    property NavigationPanel highQualityStretchingNavigationPanel: NavigationPanel {
        name: "ChangePitchHighQualityStretching"
        enabled: root.enabled && root.visible
        direction: NavigationPanel.Horizontal
        section: root.dialogView ? root.dialogView.navigationSection : null
        order: 4
    }

    QtObject {
        id: prv

        readonly property int spaceS: 4
        readonly property int spaceM: 8
        readonly property int spaceXL: 16

        readonly property int pitchDropdownFieldWidth: 128
        readonly property int fieldWidth: 98
        readonly property int desiredWidth: 480

        readonly property int borderWidth: 1
        readonly property int borderRadius: 4
    }

    Column {
        id: mainColumn

        width: parent.width

        bottomPadding: 2
        spacing: prv.spaceXL

        // Estimated start pitch label
        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            text: changePitch.estimatedStartPitch
            font: M3.typography.bodyMedium
        }

        // From pitch / To pitch section
        Column {
            width: parent.width
            spacing: prv.spaceM

            RoundedRectangle {
                width: parent.width
                height: pitchRow.height + prv.spaceXL * 2

                color: M3.color.surfaceContainer
                radius: prv.borderRadius

                border.color: M3.color.outlineVariant
                border.width: prv.borderWidth

                Row {
                    id: pitchRow

                    anchors.centerIn: parent
                    width: parent.width - prv.spaceXL * 2
                    spacing: prv.spaceXL

                    // From pitch column
                    Column {
                        width: (parent.width - parent.spacing) / 2
                        spacing: prv.spaceS

                        StyledTextLabel {
                            text: changePitch.fromPitchLabel()
                        }

                        Row {
                            width: parent.width
                            spacing: prv.spaceM

                            M3Dropdown {
                                id: fromPitchDropdown

                                width: prv.pitchDropdownFieldWidth

                                navigation.panel: root.pitchRowNavigationPanel
                                navigation.order: 0

                                currentIndex: changePitch.fromPitchValue
                                model: changePitch.fromPitchModel()

                                onActivated: function (index, value) {
                                    changePitch.fromPitchValue = index
                                }
                            }

                            M3NumberField {
                                id: fromOctaveControl

                                width: parent.width - fromPitchDropdown.width - parent.spacing

                                navigation.panel: root.pitchRowNavigationPanel
                                navigation.order: fromPitchDropdown.navigation.order + 1

                                currentValue: changePitch.fromOctaveValue
                                measureUnitsSymbol: changePitch.fromOctaveUnitSymbol()
                                decimals: changePitch.fromOctaveDecimals()
                                step: changePitch.fromOctaveStep()
                                minValue: changePitch.fromOctaveMin()
                                maxValue: changePitch.fromOctaveMax()

                                onValueEdited: function (newValue) {
                                    changePitch.fromOctaveValue = newValue
                                }
                            }
                        }
                    }

                    // To pitch column
                    Column {
                        width: (parent.width - parent.spacing) / 2
                        spacing: prv.spaceS

                        StyledTextLabel {
                            text: changePitch.toPitchLabel()
                        }

                        Row {
                            width: parent.width
                            spacing: prv.spaceM

                            M3Dropdown {
                                id: toPitchDropdown

                                width: prv.pitchDropdownFieldWidth

                                navigation.panel: root.pitchRowNavigationPanel
                                navigation.order: fromOctaveControl.navigation.order + 1

                                currentIndex: changePitch.toPitchValue
                                model: changePitch.toPitchModel()

                                onActivated: function (index, value) {
                                    changePitch.toPitchValue = index
                                }
                            }

                            M3NumberField {
                                id: toOctaveControl

                                width: parent.width - fromPitchDropdown.width - parent.spacing

                                navigation.panel: root.pitchRowNavigationPanel
                                navigation.order: toPitchDropdown.navigation.order + 1

                                currentValue: changePitch.toOctaveValue
                                measureUnitsSymbol: changePitch.toOctaveUnitSymbol()
                                decimals: changePitch.toOctaveDecimals()
                                step: changePitch.toOctaveStep()
                                minValue: changePitch.toOctaveMin()
                                maxValue: changePitch.toOctaveMax()

                                onValueEdited: function (newValue) {
                                    changePitch.toOctaveValue = newValue
                                }
                            }
                        }
                    }
                }
            }
        }

        // Semitones / Cents section
        Column {
            width: parent.width
            spacing: prv.spaceM

            RoundedRectangle {
                width: parent.width
                height: semitonesRow.height + prv.spaceXL * 2

                color: M3.color.surfaceContainer
                radius: prv.borderRadius

                border.color: M3.color.outlineVariant
                border.width: prv.borderWidth

                Row {
                    id: semitonesRow

                    anchors.centerIn: parent
                    width: parent.width - prv.spaceXL * 2
                    spacing: prv.spaceXL

                    // Semitones column
                    Column {
                        width: (parent.width - parent.spacing) / 2
                        spacing: prv.spaceS

                        StyledTextLabel {
                            text: changePitch.semitonesLabel()
                        }

                        M3NumberField {
                            id: semitonesControl

                            width: parent.width

                            navigation.panel: root.semitonesAndCentsNavigationPanel
                            navigation.order: 0

                            currentValue: changePitch.semitonesIntegerValue
                            measureUnitsSymbol: changePitch.semitonesUnitSymbol()
                            decimals: changePitch.semitonesDecimals()
                            step: changePitch.semitonesStep()
                            minValue: changePitch.semitonesMin()
                            maxValue: changePitch.semitonesMax()

                            onValueEdited: function (newValue) {
                                changePitch.semitonesIntegerValue = newValue
                            }
                        }
                    }

                    // Cents column
                    Column {
                        width: (parent.width - parent.spacing) / 2
                        spacing: prv.spaceS

                        StyledTextLabel {
                            text: changePitch.centsLabel()
                        }

                        M3NumberField {
                            id: centsControl

                            width: parent.width

                            navigation.panel: root.semitonesAndCentsNavigationPanel
                            navigation.order: semitonesControl.navigation.order + 1

                            currentValue: changePitch.centsValue
                            measureUnitsSymbol: changePitch.centsUnitSymbol()
                            decimals: changePitch.centsDecimals()
                            step: changePitch.centsStep()
                            minValue: changePitch.centsMin()
                            maxValue: changePitch.centsMax()

                            onValueEdited: function (newValue) {
                                changePitch.centsValue = newValue
                            }
                        }
                    }
                }
            }
        }

        // Frequency section
        Column {
            width: parent.width
            spacing: prv.spaceM

            RoundedRectangle {
                width: parent.width
                height: frequencyColumn.height + prv.spaceXL * 2

                color: M3.color.surfaceContainer
                radius: prv.borderRadius

                border.color: M3.color.outlineVariant
                border.width: prv.borderWidth

                Column {
                    id: frequencyColumn

                    anchors.centerIn: parent
                    width: parent.width - prv.spaceXL * 2
                    spacing: prv.spaceXL

                    // From frequency / To frequency row
                    Row {
                        width: parent.width
                        spacing: prv.spaceXL

                        // From frequency column
                        Column {
                            width: (parent.width - parent.spacing) / 2
                            spacing: prv.spaceS

                            StyledTextLabel {
                                text: changePitch.fromFrequencyLabel()
                            }

                            M3NumberField {
                                id: fromFrequencyControl

                                width: parent.width

                                navigation.panel: root.frequencyNavigationPanel
                                navigation.order: 0

                                currentValue: changePitch.fromFrequencyValue
                                measureUnitsSymbol: changePitch.fromFrequencyUnitSymbol()
                                decimals: changePitch.fromFrequencyDecimals()
                                step: changePitch.fromFrequencyStep()
                                minValue: changePitch.fromFrequencyMin()
                                maxValue: changePitch.fromFrequencyMax()

                                onValueEdited: function (newValue) {
                                    changePitch.fromFrequencyValue = newValue
                                }
                            }
                        }

                        // To frequency column
                        Column {
                            width: (parent.width - parent.spacing) / 2
                            spacing: prv.spaceS

                            StyledTextLabel {
                                text: changePitch.toFrequencyLabel()
                            }

                            M3NumberField {
                                id: toFrequencyControl

                                width: parent.width

                                navigation.panel: root.frequencyNavigationPanel
                                navigation.order: fromFrequencyControl.navigation.order + 1

                                currentValue: changePitch.toFrequencyValue
                                measureUnitsSymbol: changePitch.toFrequencyUnitSymbol()
                                decimals: changePitch.toFrequencyDecimals()
                                step: changePitch.toFrequencyStep()
                                minValue: changePitch.toFrequencyMin()
                                maxValue: changePitch.toFrequencyMax()

                                onValueEdited: function (newValue) {
                                    changePitch.toFrequencyValue = newValue
                                }
                            }
                        }
                    }

                    // Percentage change row
                    Column {
                        width: parent.width
                        spacing: prv.spaceS

                        StyledTextLabel {
                            text: changePitch.percentChangeLabel()
                        }

                        Row {
                            width: parent.width
                            spacing: prv.spaceXL

                            M3Slider {
                                id: percentSlider

                                width: parent.width - percentField.width - parent.spacing
                                anchors.verticalCenter: parent.verticalCenter

                                navigation.panel: root.frequencyNavigationPanel
                                navigation.order: toFrequencyControl.navigation.order + 1

                                from: changePitch.percentChangeMin()
                                to: changePitch.percentChangeMax()
                                value: changePitch.percentChangeValue
                                stepSize: (changePitch.percentChangeMax() - changePitch.percentChangeMin()) / 100

                                onMoved: {
                                    changePitch.percentChangeValue = value
                                }
                            }

                            M3NumberField {
                                id: percentField

                                width: prv.fieldWidth

                                navigation.panel: root.frequencyNavigationPanel
                                navigation.order: percentSlider.navigation.order + 1

                                currentValue: changePitch.percentChangeValue
                                measureUnitsSymbol: changePitch.percentChangeUnitSymbol()
                                decimals: changePitch.percentChangeDecimals()
                                step: changePitch.percentChangeStep()
                                minValue: changePitch.percentChangeMin()
                                maxValue: changePitch.percentChangeMax()

                                onValueEdited: function (newValue) {
                                    changePitch.percentChangeValue = newValue
                                }
                            }
                        }
                    }
                }
            }
        }

        // Use high quality stretching checkbox
        M3Switch {
            id: highQualityStretchingCheckbox

            width: parent.width

            navigation.panel: root.highQualityStretchingNavigationPanel
            navigation.order: 0

            text: changePitch.useSBSMSLabel()
            checked: changePitch.useSBSMSValue
            enabled: changePitch.useSBSMSEnabled()

            onToggled: {
                changePitch.useSBSMSValue = checked
                highQualityStretchingCheckbox.checked = Qt.binding(function () {
                    return changePitch.useSBSMSValue
                })
            }
        }
    }
}
