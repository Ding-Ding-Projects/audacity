/*
* Audacity: A Digital Audio Editor
*/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.ProjectScene
import Audacity.M3

import "audio"

TrackItem {
    id: root

    headerTrailingControlsComponent: Component {
        RowLayout {
            opacity: root.collapsed ? 1 : 0
            visible: opacity !== 0

            Behavior on opacity {
                OpacityAnimator {
                    duration: M3.motion.short4
                    easing: M3.motion.standard
                }
            }

            Loader {
                sourceComponent: trackControlButtons
            }
        }
    }

    extraControlsComponent: Component {
        ColumnLayout {
            spacing: 2

            RowLayout {
                id: topRow

                Layout.fillWidth: true

                spacing: 16

                opacity: root.collapsed ? 0 : 1
                visible: opacity !== 0
                Behavior on opacity {
                    OpacityAnimator {
                        duration: M3.motion.short4
                        easing: M3.motion.standard
                    }
                }

                PanKnob {
                    id: panKnob

                    value: Boolean(root.item) ? root.item.pan : 0

                    navigation.panel: root.headerNavigationPanel
                    navigation.order: root.extraControlsNavigationStart
                    navigation.enabled: !root.collapsed
                    //: Noun: accessibility name of the pan knob
                    navigation.accessible.name: qsTrc("projectscene", "Pan")

                    onNewPanRequested: function (newValue, completed) {
                        if (Boolean(root.item)) {
                            root.item.setPan(newValue, completed)
                        }
                    }
                }

                VolumeSlider {
                    id: volumeSlider

                    value: Boolean(root.item) ? root.item.volumeLevel : 0

                    navigation.panel: root.headerNavigationPanel
                    navigation.order: panKnob.navigation.order + 1
                    navigation.enabled: !root.collapsed
                    navigation.accessible.name: qsTrc("projectscene", "Track volume")

                    onNewVolumeRequested: function (newValue, completed) {
                        if (Boolean(root.item)) {
                            root.item.setVolumeLevel(newValue, completed)
                        }
                    }

                    Component.onCompleted: {
                        root.extraControlsNavigationEnd = Qt.binding(function () {
                            return volumeSlider.navigation.order
                        })
                    }
                }

                Loader {
                    sourceComponent: trackControlButtons
                }
            }

            M3Button {
                Layout.fillWidth: true
                Layout.preferredHeight: M3.density.apply(32)

                variant: "outlined"

                text: qsTrc("projectscene", "Effects")
                accessibleName: qsTrc("projectscene", "Effects")
                toolTipDescription: qsTrc("projectscene", "Opens effects panel")

                opacity: topRow.visible && root.height > root.mapFromItem(this, 0, height + bottomSeparatorHeight).y ? 1 : 0
                visible: opacity !== 0
                Behavior on opacity {
                    OpacityAnimator {
                        duration: M3.motion.short4
                        easing: M3.motion.standard
                    }
                }

                navigation.panel: root.headerNavigationPanel
                navigation.order: root.headerTrailingControlsNavigationEnd + 1
                navigation.enabled: topRow.visible

                onClicked: {
                    root.openEffectsRequested()
                }
            }
        }
    }

    rightSideContainerComponent: Component {
        Item {
            width: parent.width
            height: parent.height

            Row {
                id: volumePressureContainer

                property int indicatorWidth: 7

                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter

                spacing: 2

                function clearMeters() {
                    leftOrMonoVolumePressureMeter.reset()
                    leftOrMonoVolumePressureMeter.resetClipped()
                    rightVolumePressureMeter.reset()
                    rightVolumePressureMeter.resetClipped()
                }

                TapHandler {
                    onTapped: volumePressureContainer.clearMeters()
                }

                VolumePressureMeter {
                    id: leftOrMonoVolumePressureMeter
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    indicatorWidth: parent.indicatorWidth
                    meterModel: trackViewState.meterModel
                    currentVolumePressure: Boolean(root.item) ? root.item.leftChannelPressure : 0
                    currentRMS: Boolean(root.item) ? root.item.leftChannelRMS : 0
                }

                VolumePressureMeter {
                    id: rightVolumePressureMeter
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    indicatorWidth: parent.indicatorWidth
                    meterModel: trackViewState.meterModel
                    currentVolumePressure: Boolean(root.item) ? root.item.rightChannelPressure : 0
                    currentRMS: Boolean(root.item) ? root.item.rightChannelRMS : 0
                }

                Connections {
                    target: root.trackViewState

                    function onIsRecordingChanged() {
                        if (trackViewState.isRecording) {
                            leftOrMonoVolumePressureMeter.resetClipped()
                            rightVolumePressureMeter.resetClipped()
                        }

                        leftOrMonoVolumePressureMeter.reset()
                        rightVolumePressureMeter.reset()
                    }

                    function onIsPlayingChanged() {
                        if (trackViewState.isPlaying) {
                            volumePressureContainer.clearMeters()
                        }
                    }
                }

                Connections {
                    target: root

                    function onIsFocusedChanged() {
                        if (!root.isFocused) {
                            volumePressureContainer.clearMeters()
                        }
                    }
                }
            }

            states: [
                State {
                    when: Boolean(root.item) && root.item.channelCount === 1
                    name: "mono"
                    PropertyChanges {
                        target: volumePressureContainer
                        indicatorWidth: 8
                    }
                    PropertyChanges {
                        target: rightVolumePressureMeter
                        visible: false
                    }
                },
                State {
                    when: Boolean(root.item) && root.item.channelCount === 2
                    name: "stereo"
                    PropertyChanges {
                        target: volumePressureContainer
                        indicatorWidth: 7
                    }
                    PropertyChanges {
                        target: rightVolumePressureMeter
                        visible: true
                    }
                }
            ]
        }
    }

    Component {
        id: trackControlButtons

        RowLayout {
            spacing: 4

            M3IconButton {
                id: muteButton

                Layout.preferredWidth: M3.density.apply(32)
                Layout.preferredHeight: Layout.preferredWidth

                variant: "tonal"
                checkable: true

                icon: IconCode.MUTE
                checked: Boolean(root.item) ? root.item.muted : false

                navigation.panel: root.headerNavigationPanel
                navigation.order: root.headerTrailingControlsNavigationStart
                //: Accessibility name of the mute button
                accessibleName: qsTrc("projectscene", "Mute")
                toolTipTitle: qsTrc("projectscene", "Mute")

                onToggled: function (isChecked) {
                    if (Boolean(root.item)) {
                        root.item.muted = isChecked
                    }

                    // The button flips its own state, so the binding to the
                    // track model has to be put back.
                    muteButton.checked = Qt.binding(function () {
                        return Boolean(root.item) ? root.item.muted : false
                    })
                }
            }

            M3IconButton {
                id: soloButton

                Layout.preferredWidth: M3.density.apply(32)
                Layout.preferredHeight: Layout.preferredWidth

                variant: "tonal"
                checkable: true

                icon: IconCode.SOLO
                checked: Boolean(root.item) ? root.item.solo : false

                navigation.panel: root.headerNavigationPanel
                navigation.order: muteButton.navigation.order + 1
                //: Accessibility name of the solo button
                accessibleName: qsTrc("projectscene", "Solo")
                toolTipTitle: qsTrc("projectscene", "Solo")

                onToggled: function (isChecked) {
                    if (Boolean(root.item)) {
                        root.item.solo = isChecked
                    }

                    soloButton.checked = Qt.binding(function () {
                        return Boolean(root.item) ? root.item.solo : false
                    })
                }

                Component.onCompleted: {
                    root.headerTrailingControlsNavigationEnd = Qt.binding(function () {
                        return soloButton.navigation.order
                    })
                }
            }
        }
    }
}
