/*
* Audacity: A Digital Audio Editor
*/
import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.UiComponents

M3Slider {
    id: root

    Layout.fillWidth: true

    property double snapPoint: 0.0
    property double snapRange: 2.0
    property bool shiftPressed: false

    signal newVolumeRequested(real volume, bool completed)

    from: -60.0
    to: 12.0

    // A track header is a dense row, so the slider uses a compact anatomy and
    // shows its value in the shared value tooltip rather than in the built in
    // indicator.
    trackThickness: 6
    handleLength: 20
    handleWidth: 4
    showValueIndicator: false

    implicitHeight: 20

    accessibleName: qsTrc("projectscene", "Volume")
    valueText: root.value.toFixed(1) + " dB"

    onValueChanged: {
        root.newVolumeRequested(root.value, false)
    }

    QtObject {
        id: prv

        property bool dragActive: false
        property real innerMargin: root.handleWidth / 2
        property real startPos: 0.0
        property real startFineValue: 0.0
    }

    Connections {
        target: Qt.application

        function onStateChanged() {
            if (Qt.application.state !== Qt.ApplicationActive) {
                prv.dragActive = false
                tooltip.hide(true)
            }
        }
    }

    ValueTooltip {
        id: tooltip

        parent: root.handleItem

        unitText: "dB"
        sizingText: "-60.0dB"
        value: root.value
    }

    // Dragging is reimplemented here so that the tooltip stays visible when the
    // pointer leaves the component during a drag.
    MouseArea {
        id: mouseArea

        anchors.fill: parent

        hoverEnabled: true

        onPressed: {
            prv.dragActive = true
            root.value = mouseArea.sliderValue()
            tooltip.show(true)
            prv.startPos = mouseArea.mouseX
            prv.startFineValue = root.value
        }

        onDoubleClicked: {
            root.value = 0
            root.newVolumeRequested(0, true)
        }

        onReleased: {
            prv.dragActive = false
            if (!mouseArea.containsMouse) {
                tooltip.hide(true)
            }
            root.newVolumeRequested(root.value, true)
        }

        onEntered: {
            tooltip.show()
        }

        onExited: {
            if (!prv.dragActive) {
                tooltip.hide(true)
            }
        }

        onPositionChanged: function (e) {
            if (!prv.dragActive) {
                return
            }

            if ((e.modifiers & (Qt.ShiftModifier))) {
                if (!root.shiftPressed) {
                    root.shiftPressed = true
                    prv.startPos = mouseArea.mouseX
                    prv.startFineValue = root.value
                }

                root.value = mouseArea.fineSliderValue()
            } else {
                if (root.shiftPressed) {
                    root.shiftPressed = false
                    prv.startPos = mouseArea.mouseX
                    prv.startFineValue = root.value
                }
                root.value = mouseArea.sliderValue()
            }
        }

        onWheel: function (wheel) {
            root.step(wheel.angleDelta.y > 0 ? 1 : -1)
            root.newVolumeRequested(root.value, true)
            wheel.accepted = true
        }

        function sliderValue() {
            let relativePos = (mouseArea.mouseX - prv.innerMargin) / (mouseArea.width - root.handleWidth)
            relativePos = Math.max(0, Math.min(1, relativePos))
            return relativePos * (root.to - root.from) + root.from
        }

        function fineSliderValue() {
            let step = 2 * (mouseArea.mouseX - prv.startPos) / (root.to - root.from)
            return Math.max(root.from, Math.min(root.to, prv.startFineValue + step))
        }
    }
}
