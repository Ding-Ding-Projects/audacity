import QtQuick
import Muse.UiComponents
import Audacity.Effects
import Audacity.BuiltinEffects
import Audacity.BuiltinEffectsCollection
import Audacity.M3

BuiltinEffectBase {
    id: root

    property string title: normalizeLoudness.effectTitle
    property bool isApplyAllowed: true

    width: 400
    implicitHeight: column.height

    builtinEffectModel: NormalizeLoudnessViewModelFactory.createModel(root, root.instanceId)
    numNavigationPanels: 1
    property alias normalizeLoudness: root.builtinEffectModel
    property NavigationPanel normalizeLoudnessNavigationPanel: NavigationPanel {
        name: "NormalizeLoudnessControls"
        enabled: root.enabled && root.visible
        direction: NavigationPanel.Horizontal
        section: root.dialogView ? root.dialogView.navigationSection : null
        order: 1
    }

    Column {
        id: column

        spacing: 16

        Row {
            spacing: 6

            StyledTextLabel {
                anchors.verticalCenter: parent.verticalCenter
                text: normalizeLoudness.normalizeLabel
            }

            M3Dropdown {
                id: algorithmDropdown

                width: 194
                anchors.verticalCenter: parent.verticalCenter

                navigation.panel: root.normalizeLoudnessNavigationPanel
                navigation.order: 0

                model: normalizeLoudness.algorithmOptions
                currentIndex: normalizeLoudness.useRmsAlgorithm ? 0 : 1
                onActivated: function (index) {
                    normalizeLoudness.useRmsAlgorithm = index === 0
                }
            }

            StyledTextLabel {
                anchors.verticalCenter: parent.verticalCenter
                text: normalizeLoudness.toLabel
            }

            M3NumberField {
                id: targetControl

                width: 125
                anchors.verticalCenter: parent.verticalCenter

                navigation.panel: root.normalizeLoudnessNavigationPanel
                navigation.order: algorithmDropdown.navigation.order + 1

                measureUnitsSymbol: normalizeLoudness.currentMeasureUnitsSymbol
                step: normalizeLoudness.targetStep
                decimals: normalizeLoudness.targetDecimals
                minValue: normalizeLoudness.targetMin
                maxValue: normalizeLoudness.targetMax
                currentValue: normalizeLoudness.useRmsAlgorithm ? normalizeLoudness.rmsTarget : normalizeLoudness.perceivedLoudnessTarget
                onValueEdited: function (newValue) {
                    if (normalizeLoudness.useRmsAlgorithm) {
                        normalizeLoudness.rmsTarget = newValue
                    } else {
                        normalizeLoudness.perceivedLoudnessTarget = newValue
                    }
                }
            }
        }

        Row {
            spacing: 8

            M3Switch {
                id: independentStereoCheckbox

                navigation.panel: root.normalizeLoudnessNavigationPanel
                navigation.order: targetControl.navigation.order + 1

                checked: normalizeLoudness.normalizeStereoChannelsIndependently
                onToggled: {
                    normalizeLoudness.normalizeStereoChannelsIndependently = checked
                    independentStereoCheckbox.checked = Qt.binding(function () {
                        return normalizeLoudness.normalizeStereoChannelsIndependently
                    })
                }
            }

            StyledTextLabel {
                anchors.verticalCenter: parent.verticalCenter
                text: normalizeLoudness.independentStereoLabel
            }
        }

        Row {
            spacing: 8

            M3Switch {
                id: dualMonoCheckbox

                navigation.panel: root.normalizeLoudnessNavigationPanel
                navigation.order: independentStereoCheckbox.navigation.order + 1

                enabled: !normalizeLoudness.useRmsAlgorithm
                checked: normalizeLoudness.useDualMono
                onToggled: {
                    normalizeLoudness.useDualMono = checked
                    dualMonoCheckbox.checked = Qt.binding(function () {
                        return normalizeLoudness.useDualMono
                    })
                }
            }

            StyledTextLabel {
                anchors.verticalCenter: parent.verticalCenter
                text: normalizeLoudness.useDualMonoLabel
            }
        }
    }
}
