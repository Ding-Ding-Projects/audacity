/*
* Audacity: A Digital Audio Editor
*
* M3StateLayer
*
* The Material 3 state layer. A translucent wash of the content colour drawn
* over a container to show hover, focus, press and drag. Place it inside the
* container, above the background and below the content.
*
* Usage:
*     M3StateLayer {
*         anchors.fill: parent
*         color: M3.color.onPrimary
*         hovered: mouseArea.containsMouse
*         pressed: mouseArea.pressed
*     }
*/
import QtQuick

import Audacity.M3

Rectangle {
    id: root

    property bool hovered: false
    property bool pressed: false
    property bool focused: false
    property bool dragged: false

    // Set to false to keep the layer inert, for example while disabled.
    property bool active: true

    readonly property real targetOpacity: {
        if (!root.active) {
            return 0.0
        }
        if (root.dragged) {
            return M3.stateLayer.dragged
        }
        if (root.pressed) {
            return M3.stateLayer.pressed
        }
        if (root.hovered) {
            return M3.stateLayer.hover
        }
        if (root.focused) {
            return M3.stateLayer.focus
        }
        return 0.0
    }

    // The content colour of the component this layer sits in.
    color: M3.color.onSurface
    opacity: root.targetOpacity

    Behavior on opacity {
        NumberAnimation {
            duration: M3.motion.short3
            easing: M3.motion.standard
        }
    }
}
