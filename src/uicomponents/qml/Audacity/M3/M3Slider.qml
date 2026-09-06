/*
* Audacity: A Digital Audio Editor
*
* M3Slider
*
* The Material 3 Expressive slider: a tall pill handle, an active track in the
* primary colour, an inactive track in the secondary container colour, optional
* tick marks for a discrete slider and a value indicator above the handle while
* dragging. Works horizontally and vertically.
*
* Replaces: Muse.UiComponents StyledSlider.
*
* API:
*     value, from, to, stepSize, orientation, showTicks, showValueIndicator,
*     valueText, moved(), navigation
*
* The three metrics of the anatomy are properties rather than constants, so a
* compact slider in a track header can shrink the track and the handle without
* the component being forked. handleItem gives a caller something to anchor a
* value tooltip to.
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FocusScope {
    id: root

    property real value: 0.0
    property real from: 0.0
    property real to: 1.0

    // Zero means a continuous slider.
    property real stepSize: 0.0

    property int orientation: Qt.Horizontal

    property bool showTicks: root.stepSize > 0.0
    property bool showValueIndicator: true

    property string accessibleName: ""
    // Override to format the value indicator, for example as decibels.
    property string valueText: root.value.toFixed(root.stepSize > 0 ? 0 : 2)

    property alias navigation: navCtrl

    // Personalize appearance override hookup, see M3Button.qml for detail.
    property string elementId: ""
    property int appearanceRevision: 0

    function m3Appearance(property, fallback) {
        root.appearanceRevision
        if (root.elementId === "" || typeof AppearanceOverrides === "undefined") {
            return fallback
        }
        return AppearanceOverrides.resolve(root.elementId, "", property, fallback)
    }

    Connections {
        target: typeof AppearanceOverrides !== "undefined" ? AppearanceOverrides : null
        ignoreUnknownSignals: true

        function onElementChanged(elementId) {
            if (elementId === root.elementId) {
                root.appearanceRevision = root.appearanceRevision + 1
            }
        }
    }

    signal moved

    readonly property bool horizontal: root.orientation === Qt.Horizontal
    readonly property real range: root.to - root.from
    readonly property real position: root.range === 0 ? 0 : (root.value - root.from) / root.range

    property real trackThickness: 16
    property real handleWidth: 4
    property real handleLength: 44

    property alias handleItem: handle

    implicitWidth: root.horizontal ? 200 : root.handleLength
    implicitHeight: root.horizontal ? root.handleLength : 200

    readonly property real travelLength: (root.horizontal ? root.width : root.height) - root.handleWidth * 2

    function clampValue(candidate) {
        var bounded = Math.max(root.from, Math.min(root.to, candidate))
        if (root.stepSize > 0) {
            bounded = root.from + Math.round((bounded - root.from) / root.stepSize) * root.stepSize
        }
        return Math.max(root.from, Math.min(root.to, bounded))
    }

    function setValue(candidate) {
        var next = root.clampValue(candidate)
        if (next !== root.value) {
            root.value = next
            root.moved()
        }
    }

    function step(direction) {
        var amount = root.stepSize > 0 ? root.stepSize : root.range / 100
        root.setValue(root.value + amount * direction)
    }

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3Slider"
        enabled: root.enabled && root.visible
        accessible.role: MUAccessible.Range
        accessible.name: root.accessibleName
        accessible.value: root.value
        accessible.minimumValue: root.from
        accessible.maximumValue: root.to
        accessible.stepSize: root.stepSize

        onActiveChanged: {
            if (navCtrl.active && !root.activeFocus) {
                root.forceActiveFocus()
            }
        }
    }

    Keys.onLeftPressed: root.step(root.horizontal ? -1 : 0)
    Keys.onRightPressed: root.step(root.horizontal ? 1 : 0)
    Keys.onDownPressed: root.step(-1)
    Keys.onUpPressed: root.step(1)
    Keys.onPressed: function (event) {
        if (event.key === Qt.Key_Home) {
            root.setValue(root.from)
            event.accepted = true
        } else if (event.key === Qt.Key_End) {
            root.setValue(root.to)
            event.accepted = true
        }
    }

    // Inactive track.
    Rectangle {
        id: inactiveTrack

        anchors.centerIn: parent
        width: root.horizontal ? root.width : root.trackThickness
        height: root.horizontal ? root.trackThickness : root.height
        radius: root.trackThickness / 2
        antialiasing: true
        color: root.enabled ? M3.color.secondaryContainer : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContainer)
    }

    // Active track.
    Rectangle {
        id: activeTrack

        radius: root.trackThickness / 2
        antialiasing: true
        readonly property color defaultColor: root.enabled ? M3.color.primary : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
        color: root.m3Appearance("containerColor", activeTrack.defaultColor)

        x: root.horizontal ? 0 : (root.width - root.trackThickness) / 2
        y: root.horizontal ? (root.height - root.trackThickness) / 2 : root.height - activeTrack.height
        width: root.horizontal ? handle.x + root.handleWidth : root.trackThickness
        height: root.horizontal ? root.trackThickness : root.height - handle.y - root.handleWidth
    }

    // Tick marks for a discrete slider.
    Repeater {
        id: ticks

        readonly property int count: root.showTicks && root.stepSize > 0 ? Math.floor(root.range / root.stepSize) + 1 : 0

        model: ticks.count

        delegate: Rectangle {
            required property int index

            readonly property real fraction: ticks.count <= 1 ? 0 : index / (ticks.count - 1)
            readonly property bool past: fraction <= root.position

            width: 4
            height: 4
            radius: 2
            antialiasing: true
            color: past ? M3.color.onPrimary : M3.color.onSecondaryContainer
            opacity: root.enabled ? 1.0 : M3.stateLayer.disabledContent

            x: root.horizontal ? root.handleWidth + fraction * root.travelLength - 2 : (root.width - 4) / 2
            y: root.horizontal ? (root.height - 4) / 2 : root.height - root.handleWidth - fraction * root.travelLength - 2
        }
    }

    // The Expressive handle: a tall narrow pill.
    Rectangle {
        id: handle

        width: root.horizontal ? root.handleWidth : root.handleLength
        height: root.horizontal ? root.handleLength : root.handleWidth
        radius: root.m3Appearance("radius", root.handleWidth / 2)
        antialiasing: true
        readonly property color defaultColor: root.enabled ? M3.color.primary : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
        color: root.m3Appearance("handleColor", handle.defaultColor)

        x: root.horizontal ? root.handleWidth + root.position * root.travelLength - root.handleWidth / 2 : (root.width - handle.width) / 2
        y: root.horizontal ? (root.height - handle.height) / 2 : root.height - root.handleWidth - root.position * root.travelLength - root.handleWidth / 2

        Behavior on x {
            enabled: !mouseArea.pressed
            NumberAnimation {
                duration: M3.motion.short3
                easing: M3.motion.standard
            }
        }

        Behavior on y {
            enabled: !mouseArea.pressed
            NumberAnimation {
                duration: M3.motion.short3
                easing: M3.motion.standard
            }
        }
    }

    M3FocusRing {
        anchors.fill: handle
        shapeRadius: handle.radius
        visible: navCtrl.highlight
    }

    // Value indicator, shown while the handle is being dragged.
    Rectangle {
        id: valueIndicator

        visible: root.showValueIndicator && mouseArea.pressed
        color: M3.color.inverseSurface
        radius: M3.shape.full
        antialiasing: true

        implicitWidth: Math.max(28, valueLabel.implicitWidth + 16)
        implicitHeight: 28
        width: implicitWidth
        height: implicitHeight

        x: root.horizontal ? Math.max(0, Math.min(root.width - width, handle.x + handle.width / 2 - width / 2)) : handle.x - width - 8
        y: root.horizontal ? handle.y - height - 8 : Math.max(0, Math.min(root.height - height, handle.y + handle.height / 2 - height / 2))

        StyledTextLabel {
            id: valueLabel

            anchors.centerIn: parent
            text: root.valueText
            font: M3.typography.labelLarge
            color: M3.color.inverseOnSurface
        }
    }

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled
        cursorShape: Qt.PointingHandCursor

        function valueAt(mouse) {
            var fraction = root.horizontal ? (mouse.x - root.handleWidth) / root.travelLength : 1.0 - (mouse.y - root.handleWidth) / root.travelLength
            return root.from + Math.max(0, Math.min(1, fraction)) * root.range
        }

        onPressed: function (mouse) {
            navCtrl.requestActive()
            root.setValue(mouseArea.valueAt(mouse))
        }

        onPositionChanged: function (mouse) {
            if (mouseArea.pressed) {
                root.setValue(mouseArea.valueAt(mouse))
            }
        }

        onWheel: function (wheel) {
            root.step(wheel.angleDelta.y > 0 ? 1 : -1)
            wheel.accepted = true
        }
    }
}
