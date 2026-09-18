/*
* Audacity: A Digital Audio Editor
*
* M3Surface
*
* A tonal surface. It picks the right container colour for an elevation level,
* draws the matching shadow and applies a corner radius from the shape scale.
* Use it as the background of any panel, card, sheet or menu.
*
* Usage:
*     M3Surface {
*         anchors.fill: parent
*         level: 2
*         radius: M3.shape.large
*     }
*/
import QtQuick

import Audacity.M3

Rectangle {
    id: root

    // Elevation level, 0 to 5. Drives both the tonal tint and the shadow.
    property int level: 0
    // Set to false for a surface that should not cast a shadow, such as a dock.
    property bool shadowVisible: true
    property bool outlined: false
    property color outlineColor: M3.color.outlineVariant

    // Identifies this surface to the personalize appearance editor's layer
    // stack. Left empty (the default) no layer rendering happens at all, so
    // an ordinary surface pays nothing for this. visualState picks which
    // per-state stack is used ("hover", "pressed", and so on); it is left to
    // the owning component to set, since M3Surface itself has no notion of
    // interaction state.
    property string elementId: ""
    property string visualState: ""

    color: M3.surfaceAt(root.level)
    radius: M3.shape.none
    border.width: root.outlined ? 1 : 0
    border.color: root.outlined ? root.outlineColor : "transparent"
    antialiasing: true

    M3Elevation {
        anchors.fill: parent
        level: root.shadowVisible ? root.level : 0
        radius: root.radius
    }

    M3AppearanceLayers {
        anchors.fill: parent
        elementId: root.elementId
        appearanceState: root.visualState
        radius: root.radius
    }
}
