/*
* Audacity: A Digital Audio Editor
*
* M3RangeSlider
*
* Two Material 3 Expressive handles selecting a span inside one track. The
* handles cannot cross. Each handle carries its own navigation control so that
* the keyboard can reach both ends.
*
* API:
*     first, second, from, to, stepSize, orientation, moved(), navigationPanel
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property real first: 0.0
    property real second: 1.0
    property real from: 0.0
    property real to: 1.0
    property real stepSize: 0.0

    property int orientation: Qt.Horizontal

    property string firstAccessibleName: "Range start"
    property string secondAccessibleName: "Range end"

    property NavigationPanel navigationPanel: null
    property int navigationRow: 0

    signal moved()

    readonly property bool horizontal: root.orientation === Qt.Horizontal
    readonly property real range: root.to - root.from
    readonly property real trackThickness: 16
    readonly property real handleWidth: 4
    readonly property real handleLength: 44

    implicitWidth: root.horizontal ? 240 : root.handleLength
    implicitHeight: root.horizontal ? root.handleLength : 240

    readonly property real travelLength: (root.horizontal ? root.width : root.height) - root.handleWidth * 2

    function fractionOf(value) {
        return root.range === 0 ? 0 : (value - root.from) / root.range
    }

    function snap(candidate) {
        var bounded = Math.max(root.from, Math.min(root.to, candidate))
        if (root.stepSize > 0) {
            bounded = root.from + Math.round((bounded - root.from) / root.stepSize) * root.stepSize
        }
        return Math.max(root.from, Math.min(root.to, bounded))
    }

    function setFirst(candidate) {
        var next = Math.min(root.snap(candidate), root.second)
        if (next !== root.first) {
            root.first = next
            root.moved()
        }
    }

    function setSecond(candidate) {
        var next = Math.max(root.snap(candidate), root.first)
        if (next !== root.second) {
            root.second = next
            root.moved()
        }
    }

    function positionFor(value) {
        var fraction = root.fractionOf(value)
        return root.horizontal
                ? root.handleWidth + fraction * root.travelLength
                : root.height - root.handleWidth - fraction * root.travelLength
    }

    Rectangle {
        id: inactiveTrack

        anchors.centerIn: parent
        width: root.horizontal ? root.width : root.trackThickness
        height: root.horizontal ? root.trackThickness : root.height
        radius: root.trackThickness / 2
        antialiasing: true
        color: root.enabled ? M3.color.secondaryContainer
                            : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g,
                                      M3.color.onSurface.b, M3.stateLayer.disabledContainer)
    }

    Rectangle {
        id: activeTrack

        radius: root.trackThickness / 2
        antialiasing: true
        color: root.enabled ? M3.color.primary
                            : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g,
                                      M3.color.onSurface.b, M3.stateLayer.disabledContent)

        x: root.horizontal ? root.positionFor(root.first) : (root.width - root.trackThickness) / 2
        y: root.horizontal ? (root.height - root.trackThickness) / 2 : root.positionFor(root.second)
        width: root.horizontal
               ? root.positionFor(root.second) - root.positionFor(root.first)
               : root.trackThickness
        height: root.horizontal
                ? root.trackThickness
                : root.positionFor(root.first) - root.positionFor(root.second)
    }

    component RangeHandle: Rectangle {
        id: handle

        property real handleValue: 0
        property bool active: false

        width: root.horizontal ? root.handleWidth : root.handleLength
        height: root.horizontal ? root.handleLength : root.handleWidth
        radius: root.handleWidth / 2
        antialiasing: true
        color: root.enabled ? M3.color.primary
                            : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g,
                                      M3.color.onSurface.b, M3.stateLayer.disabledContent)

        x: root.horizontal ? root.positionFor(handle.handleValue) - root.handleWidth / 2
                           : (root.width - handle.width) / 2
        y: root.horizontal ? (root.height - handle.height) / 2
                           : root.positionFor(handle.handleValue) - root.handleWidth / 2
    }

    RangeHandle {
        id: firstHandle
        handleValue: root.first
    }

    RangeHandle {
        id: secondHandle
        handleValue: root.second
    }

    NavigationControl {
        id: firstNav

        name: "M3RangeSliderFirst"
        panel: root.navigationPanel
        row: root.navigationRow
        column: 0
        enabled: root.enabled && root.visible
        accessible.role: MUAccessible.Range
        accessible.name: root.firstAccessibleName
        accessible.visualItem: firstHandle
        accessible.value: root.first
        accessible.minimumValue: root.from
        accessible.maximumValue: root.second
        accessible.stepSize: root.stepSize
    }

    NavigationControl {
        id: secondNav

        name: "M3RangeSliderSecond"
        panel: root.navigationPanel
        row: root.navigationRow
        column: 1
        enabled: root.enabled && root.visible
        accessible.role: MUAccessible.Range
        accessible.name: root.secondAccessibleName
        accessible.visualItem: secondHandle
        accessible.value: root.second
        accessible.minimumValue: root.first
        accessible.maximumValue: root.to
        accessible.stepSize: root.stepSize
    }

    M3FocusRing {
        anchors.fill: firstHandle
        shapeRadius: firstHandle.radius
        visible: firstNav.highlight
    }

    M3FocusRing {
        anchors.fill: secondHandle
        shapeRadius: secondHandle.radius
        visible: secondNav.highlight
    }

    Keys.onLeftPressed: root.nudge(-1)
    Keys.onRightPressed: root.nudge(1)
    Keys.onDownPressed: root.nudge(-1)
    Keys.onUpPressed: root.nudge(1)

    function nudge(direction) {
        var amount = root.stepSize > 0 ? root.stepSize : root.range / 100
        if (secondNav.active) {
            root.setSecond(root.second + amount * direction)
        } else {
            root.setFirst(root.first + amount * direction)
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled

        property bool draggingSecond: false

        function valueAt(mouse) {
            var fraction = root.horizontal
                    ? (mouse.x - root.handleWidth) / root.travelLength
                    : 1.0 - (mouse.y - root.handleWidth) / root.travelLength
            return root.from + Math.max(0, Math.min(1, fraction)) * root.range
        }

        onPressed: function(mouse) {
            var candidate = mouseArea.valueAt(mouse)
            mouseArea.draggingSecond = Math.abs(candidate - root.second) < Math.abs(candidate - root.first)
            if (mouseArea.draggingSecond) {
                secondNav.requestActive()
                root.setSecond(candidate)
            } else {
                firstNav.requestActive()
                root.setFirst(candidate)
            }
        }

        onPositionChanged: function(mouse) {
            if (!mouseArea.pressed) {
                return
            }
            var candidate = mouseArea.valueAt(mouse)
            if (mouseArea.draggingSecond) {
                root.setSecond(candidate)
            } else {
                root.setFirst(candidate)
            }
        }
    }
}
