/*
* Audacity: A Digital Audio Editor
*
* M3Knob
*
* A Material 3 rotary knob. The inactive track is an arc in the surface
* container highest role, the active arc is drawn in the primary role from the
* origin of the range, and a pill handle points at the current value. A
* circular state layer covers the whole dial, the focus ring follows the dial
* outline and a value indicator appears above the knob while it is being
* dragged, in the same anatomy the slider uses.
*
* A bidirectional knob, such as a pan control, grows its active arc out of the
* twelve o'clock position in either direction. A unidirectional knob grows it
* from the start of the sweep.
*
* API:
*     value, from, to, stepSize, radius, bidirectional, accentControl,
*     valueText, showValueIndicator, mouseArea, navigation,
*     newValueRequested(value), moved(), mouseEntered(), mouseExited(),
*     mousePressed(), mouseReleased()
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
    property real stepSize: 0.0

    // Radius of the dial. The component is twice this in both directions.
    property real radius: 16

    // Draws the active arc out of the twelve o'clock position in either
    // direction rather than out of the start of the sweep.
    property bool bidirectional: false

    // A knob that is not an accent control draws its active arc in a muted
    // on-surface tone instead of the primary role.
    property bool accentControl: true

    property string accessibleName: ""
    property string valueText: root.value.toFixed(root.stepSize > 0 && root.stepSize < 1 ? 1 : 0)
    property bool showValueIndicator: false

    property alias navigation: navCtrl
    property alias mouseArea: mouseArea

    signal newValueRequested(real newValue)
    signal moved
    signal mouseEntered
    signal mouseExited
    signal mousePressed
    signal mouseReleased

    implicitWidth: root.radius * 2
    implicitHeight: root.radius * 2

    width: implicitWidth
    height: implicitHeight

    readonly property real range: root.to - root.from
    readonly property real position: root.range === 0 ? 0 : (root.value - root.from) / root.range

    // The sweep runs from minus 140 to plus 140 degrees around the top.
    readonly property real sweepDegrees: 140
    readonly property real angle: -root.sweepDegrees + root.position * root.sweepDegrees * 2

    function clampValue(candidate) {
        var bounded = Math.max(root.from, Math.min(root.to, candidate))
        if (root.stepSize > 0) {
            bounded = root.from + Math.round((bounded - root.from) / root.stepSize) * root.stepSize
        }
        return Math.max(root.from, Math.min(root.to, bounded))
    }

    function requestNewValue(candidate) {
        var next = root.clampValue(candidate)
        if (next === root.value) {
            return
        }
        root.newValueRequested(next)
        root.moved()
    }

    function step(direction) {
        var amount = root.stepSize > 0 ? root.stepSize : root.range / 100
        root.requestNewValue(root.value + amount * direction)
    }

    QtObject {
        id: prv

        readonly property real handleHeight: root.radius / 2
        readonly property real handleWidth: Math.max(2, root.radius / 8)

        readonly property real activeArcWidth: root.radius / 5
        readonly property real innerArcWidth: root.radius / 8

        readonly property real startAngle: -root.sweepDegrees * (Math.PI / 180) - Math.PI / 2
        readonly property real endAngle: root.sweepDegrees * (Math.PI / 180) - Math.PI / 2

        readonly property color activeArcColor: root.enabled ? (root.accentControl ? M3.color.primary : Utils.colorWithAlpha(M3.color.onSurface, 0.3)) : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)
        readonly property color trackArcColor: root.enabled ? M3.color.surfaceContainerHighest : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContainer)
        readonly property color innerArcColor: Utils.colorWithAlpha(M3.color.outlineVariant, 0.8)

        readonly property real originDegrees: root.bidirectional ? 0 : -root.sweepDegrees
        readonly property bool reversed: root.bidirectional ? root.angle < 0 : false

        property real prevX: 0
        property real prevY: 0
        property real unclampedValue: -1
        property bool shiftPressed: false
        property bool dragActive: false

        onActiveArcColorChanged: dialCanvas.requestPaint()
        onTrackArcColorChanged: dialCanvas.requestPaint()
        onInnerArcColorChanged: dialCanvas.requestPaint()
    }

    NavigationControl {
        id: navCtrl

        name: root.objectName !== "" ? root.objectName : "M3Knob"
        enabled: root.enabled && root.visible

        accessible.role: MUAccessible.Range
        accessible.name: root.accessibleName
        accessible.visualItem: dialCanvas

        accessible.value: root.value
        accessible.minimumValue: root.from
        accessible.maximumValue: root.to
        accessible.stepSize: root.stepSize

        onActiveChanged: {
            if (navCtrl.active && !root.activeFocus) {
                root.forceActiveFocus()
            }
        }

        onNavigationEvent: function (event) {
            switch (event.type) {
            case NavigationEvent.Up:
                root.step(1)
                event.accepted = true
                break
            case NavigationEvent.Down:
                root.step(-1)
                event.accepted = true
                break
            }
        }
    }

    Keys.onLeftPressed: root.step(-1)
    Keys.onRightPressed: root.step(1)
    Keys.onDownPressed: root.step(-1)
    Keys.onUpPressed: root.step(1)
    Keys.onPressed: function (event) {
        if (event.key === Qt.Key_Home) {
            root.requestNewValue(root.from)
            event.accepted = true
        } else if (event.key === Qt.Key_End) {
            root.requestNewValue(root.to)
            event.accepted = true
        }
    }

    M3StateLayer {
        anchors.centerIn: dialCanvas
        width: root.radius * 2
        height: root.radius * 2
        radius: root.radius
        color: root.accentControl ? M3.color.primary : M3.color.onSurface
        active: root.enabled
        hovered: mouseArea.containsMouse
        pressed: mouseArea.containsPress
        focused: navCtrl.highlight
    }

    Canvas {
        id: dialCanvas

        anchors.centerIn: parent
        width: root.radius * 2
        height: root.radius * 2

        antialiasing: true

        onPaint: {
            var ctx = dialCanvas.context
            if (!ctx) {
                ctx = dialCanvas.getContext("2d")
                ctx.lineCap = "round"
            }

            ctx.clearRect(0, 0, dialCanvas.width, dialCanvas.height);

            // Inactive track arc.
            ctx.lineWidth = prv.activeArcWidth
            ctx.strokeStyle = prv.trackArcColor
            ctx.beginPath()
            ctx.arc(dialCanvas.width / 2, dialCanvas.height / 2, root.radius - prv.activeArcWidth / 2, prv.startAngle, prv.endAngle, false)
            ctx.stroke();

            // Active arc.
            ctx.lineWidth = prv.activeArcWidth
            ctx.strokeStyle = prv.activeArcColor
            ctx.beginPath()
            var activeStart = prv.originDegrees * (Math.PI / 180) - Math.PI / 2
            var activeEnd = root.angle * (Math.PI / 180) - Math.PI / 2
            ctx.arc(dialCanvas.width / 2, dialCanvas.height / 2, root.radius - prv.activeArcWidth / 2, activeStart, activeEnd, prv.reversed)
            ctx.stroke();

            // Inner outline that separates the dial face from the arcs.
            ctx.lineWidth = prv.innerArcWidth
            ctx.strokeStyle = prv.innerArcColor
            ctx.beginPath()
            ctx.arc(dialCanvas.width / 2, dialCanvas.height / 2, root.radius - (prv.activeArcWidth + prv.innerArcWidth / 2), 0, Math.PI * 2, false)
            ctx.stroke()
        }
    }

    // The handle, a pill pointing from the centre towards the current value.
    Rectangle {
        id: handle

        x: dialCanvas.x + root.radius - prv.handleWidth / 2
        y: dialCanvas.y + prv.activeArcWidth + prv.innerArcWidth + 2

        width: prv.handleWidth
        height: prv.handleHeight
        radius: prv.handleWidth / 2
        antialiasing: true

        color: root.enabled ? M3.color.onSurface : Qt.rgba(M3.color.onSurface.r, M3.color.onSurface.g, M3.color.onSurface.b, M3.stateLayer.disabledContent)

        transformOrigin: Item.Bottom
        rotation: root.angle
    }

    M3FocusRing {
        anchors.fill: dialCanvas
        shapeRadius: root.radius
        visible: navCtrl.highlight
    }

    // Value indicator, shown while the dial is being turned.
    Rectangle {
        id: valueIndicator

        visible: root.showValueIndicator && prv.dragActive
        color: M3.color.inverseSurface
        radius: M3.shape.full
        antialiasing: true

        width: Math.max(28, valueLabel.implicitWidth + 16)
        height: 28

        x: (root.width - width) / 2
        y: -height - 8

        StyledTextLabel {
            id: valueLabel

            anchors.centerIn: parent
            text: root.valueText
            font: M3.typography.labelLarge
            color: M3.color.inverseOnSurface
        }
    }

    onValueChanged: dialCanvas.requestPaint()
    onEnabledChanged: dialCanvas.requestPaint()
    Component.onCompleted: dialCanvas.requestPaint()

    MouseArea {
        id: mouseArea

        anchors.fill: parent
        hoverEnabled: true
        enabled: root.enabled

        // Don't let a Flickable steal the mouse while the dial is being turned.
        preventStealing: true

        onDoubleClicked: root.requestNewValue(0)

        onPressed: function (mouse) {
            navCtrl.requestActive()
            prv.prevX = mouse.x
            prv.prevY = mouse.y
            prv.dragActive = true
            root.mousePressed()
        }

        onEntered: root.mouseEntered()

        onExited: {
            // The pointer counts as still inside while a drag is in progress.
            if (!prv.dragActive) {
                root.mouseExited()
            }
        }

        onReleased: {
            prv.dragActive = false
            prv.unclampedValue = -1
            if (!mouseArea.containsMouse) {
                root.mouseExited()
            }
            root.mouseReleased()
        }

        onPositionChanged: function (mouse) {
            if (!prv.dragActive) {
                return
            }

            var fine = (mouse.modifiers & Qt.ShiftModifier) !== 0
            if (fine !== prv.shiftPressed) {
                prv.shiftPressed = fine
                prv.unclampedValue = -1
            }

            var dx = mouse.x - prv.prevX
            var dy = mouse.y - prv.prevY
            var dist = Math.sqrt(dx * dx + dy * dy)
            if (fine) {
                dist /= 3
            }

            var sgn = (dy < dx) ? 1 : -1
            var next = (prv.unclampedValue === -1 ? root.value : prv.unclampedValue) + root.range * dist / 200 * sgn

            prv.prevX = mouse.x
            prv.prevY = mouse.y
            prv.unclampedValue = next

            root.requestNewValue(next)
        }

        onWheel: function (wheel) {
            root.step(wheel.angleDelta.y > 0 ? 1 : -1)
            wheel.accepted = true
        }
    }
}
