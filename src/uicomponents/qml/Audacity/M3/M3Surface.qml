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
}
