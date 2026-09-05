/*
* Audacity: A Digital Audio Editor
*
* M3CircularProgress
*
* A Material 3 circular progress indicator, determinate or indeterminate. The
* Expressive variant draws a wavy ring. Under reduced motion the indeterminate
* form settles into a static three quarter arc that pulses gently.
*
* API:
*     value, from, to, indeterminate, wavy, implicitSize, indicatorColor
*/
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Shapes

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Item {
    id: root

    property real value: 0.0
    property real from: 0.0
    property real to: 1.0

    property bool indeterminate: true
    property bool wavy: false
    property bool running: true

    property real implicitSize: 48
    property real strokeWidth: 4

    property color indicatorColor: M3.color.primary
    property color trackColor: M3.color.secondaryContainer

    property string accessibleName: "Progress"

    readonly property real progress: root.to === root.from
                                     ? 0 : Math.max(0, Math.min(1, (root.value - root.from) / (root.to - root.from)))

    implicitWidth: root.implicitSize
    implicitHeight: root.implicitSize

    Shape {
        id: shape

        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        property real spin: 0
        property real radius: Math.min(root.width, root.height) / 2 - root.strokeWidth / 2 - (root.wavy ? 2 : 0)

        NumberAnimation on spin {
            running: root.running && root.indeterminate && !M3.motion.reducedMotion
            loops: Animation.Infinite
            from: 0
            to: 360
            duration: M3.motion.extraLong2
        }

        ShapePath {
            strokeColor: root.trackColor
            strokeWidth: root.strokeWidth
            fillColor: "transparent"

            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: shape.radius
                radiusY: shape.radius
                startAngle: 0
                sweepAngle: 360
            }
        }

        ShapePath {
            strokeColor: root.indicatorColor
            strokeWidth: root.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: shape.radius
                radiusY: shape.radius
                startAngle: -90 + shape.spin
                sweepAngle: root.indeterminate ? 270 : 360 * root.progress
            }
        }
    }

    // Reduced motion replaces the spin with a gentle pulse.
    SequentialAnimation on opacity {
        running: root.running && root.indeterminate && M3.motion.reducedMotion
        loops: Animation.Infinite

        NumberAnimation { from: 0.5; to: 1.0; duration: 600 }
        NumberAnimation { from: 1.0; to: 0.5; duration: 600 }
    }

    AccessibleItem {
        visualItem: root
        role: MUAccessible.Range
        name: root.accessibleName
        value: Math.round(root.progress * 100)
        minimumValue: 0
        maximumValue: 100
        enabled: root.visible && !root.indeterminate
    }
}
