/*
* Audacity: A Digital Audio Editor
*
* M3Ripple
*
* A single expanding circle that starts where the pointer went down and fades
* out when it comes back up. Honours reduced motion by fading in place instead
* of travelling.
*
* Usage:
*     M3Ripple {
*         id: ripple
*         anchors.fill: parent
*         color: M3.color.onPrimary
*     }
*     TapHandler { onPressedChanged: ripple.press(point.position) }
*/
import QtQuick

import Audacity.M3

Item {
    id: root

    property color color: M3.color.onSurface
    property real maxOpacity: M3.stateLayer.pressed

    // Start a ripple at a point in this item's coordinate system.
    function press(position) {
        if (!root.enabled) {
            return
        }
        circle.x = position.x
        circle.y = position.y
        rippleAnimation.restart()
    }

    // Start a ripple in the centre, for keyboard activation.
    function pulse() {
        root.press(Qt.point(root.width / 2, root.height / 2))
    }

    clip: true

    readonly property real maxRadius: Math.sqrt(root.width * root.width + root.height * root.height)

    Rectangle {
        id: circle

        width: 0
        height: 0
        radius: width / 2
        opacity: 0
        color: root.color

        transform: Translate {
            x: -circle.width / 2
            y: -circle.height / 2
        }
    }

    SequentialAnimation {
        id: rippleAnimation

        ParallelAnimation {
            NumberAnimation {
                target: circle
                property: "width"
                from: M3.motion.reducedMotion ? root.maxRadius * 2 : 0
                to: root.maxRadius * 2
                duration: M3.motion.medium2
                easing: M3.motion.standardDecelerate
            }
            NumberAnimation {
                target: circle
                property: "height"
                from: M3.motion.reducedMotion ? root.maxRadius * 2 : 0
                to: root.maxRadius * 2
                duration: M3.motion.medium2
                easing: M3.motion.standardDecelerate
            }
            NumberAnimation {
                target: circle
                property: "opacity"
                from: root.maxOpacity
                to: root.maxOpacity
                duration: M3.motion.short2
            }
        }

        NumberAnimation {
            target: circle
            property: "opacity"
            to: 0
            duration: M3.motion.medium1
            easing: M3.motion.standardAccelerate
        }
    }
}
