/*
* Audacity: A Digital Audio Editor
*
* M3LinearProgress
*
* A Material 3 linear progress indicator, determinate or indeterminate, with an
* optional Expressive wavy active track. The wave settles to a straight line
* under reduced motion.
*
* Replaces: Muse.UiComponents ProgressBar.
*
* API:
*     value, from, to, indeterminate, wavy, accessibleName
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

    property bool indeterminate: false

    // The Material 3 Expressive wavy active track.
    property bool wavy: false

    property string accessibleName: "Progress"

    readonly property real progress: root.to === root.from ? 0 : Math.max(0, Math.min(1, (root.value - root.from) / (root.to - root.from)))

    implicitHeight: 4
    implicitWidth: 240

    readonly property bool animateWave: root.wavy && !M3.motion.reducedMotion

    Rectangle {
        id: inactiveTrack

        anchors.fill: parent
        radius: height / 2
        antialiasing: true
        color: M3.color.secondaryContainer
    }

    // Determinate straight track.
    Rectangle {
        id: activeTrack

        visible: !root.indeterminate && !root.animateWave
        height: parent.height
        width: parent.width * root.progress
        radius: height / 2
        antialiasing: true
        color: M3.color.primary

        Behavior on width {
            NumberAnimation {
                duration: M3.motion.medium1
                easing: M3.motion.standard
            }
        }
    }

    // Determinate wavy track.
    Shape {
        id: wave

        visible: !root.indeterminate && root.animateWave
        anchors.fill: parent
        preferredRendererType: Shape.CurveRenderer

        property real phase: 0

        NumberAnimation on phase {
            running: wave.visible
            loops: Animation.Infinite
            from: 0
            to: 2 * Math.PI
            duration: M3.motion.extraLong2
        }

        ShapePath {
            strokeColor: M3.color.primary
            strokeWidth: 3
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathPolyline {
                path: {
                    var points = []
                    var width = root.width * root.progress
                    var midY = root.height / 2
                    for (var x = 0; x <= width; x += 2) {
                        points.push(Qt.point(x, midY + Math.sin(x / 8 + wave.phase) * 1.5))
                    }
                    if (points.length === 0) {
                        points.push(Qt.point(0, midY))
                    }
                    return points
                }
            }
        }
    }

    // Indeterminate sweep.
    Rectangle {
        id: sweep

        visible: root.indeterminate
        height: parent.height
        width: parent.width * 0.4
        radius: height / 2
        antialiasing: true
        color: M3.color.primary

        SequentialAnimation on x {
            running: sweep.visible && !M3.motion.reducedMotion
            loops: Animation.Infinite

            NumberAnimation {
                from: -sweep.width
                to: root.width
                duration: M3.motion.extraLong2
                easing: M3.motion.standard
            }
        }

        // Under reduced motion the indeterminate bar pulses in place instead.
        SequentialAnimation on opacity {
            running: sweep.visible && M3.motion.reducedMotion
            loops: Animation.Infinite

            NumberAnimation {
                from: 0.4
                to: 1.0
                duration: 600
            }
            NumberAnimation {
                from: 1.0
                to: 0.4
                duration: 600
            }
        }
    }

    AccessibleItem {
        id: accessibleInfo

        visualItem: root
        role: MUAccessible.Range
        name: root.accessibleName
        value: Math.round(root.progress * 100)
        minimumValue: 0
        maximumValue: 100
        enabled: root.visible
    }
}
