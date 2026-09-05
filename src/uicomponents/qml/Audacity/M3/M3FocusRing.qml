/*
* Audacity: A Digital Audio Editor
*
* M3FocusRing
*
* The Material 3 focus indicator: a three pixel primary coloured ring drawn two
* pixels outside the shape it belongs to. Anchor it to the component it marks
* and give it the same corner radius.
*
* Usage:
*     M3FocusRing {
*         anchors.fill: parent
*         shapeRadius: M3.shape.full
*         visible: root.navigation.highlight
*     }
*/
import QtQuick

import Audacity.M3

Rectangle {
    id: root

    // Corner radius of the shape this ring surrounds, before the outward offset.
    property real shapeRadius: 0
    property color ringColor: M3.color.primary

    readonly property real thickness: M3.focusIndicatorThickness
    readonly property real offset: M3.focusIndicatorOffset

    anchors.margins: -(root.offset + root.thickness)

    color: "transparent"
    border.width: root.thickness
    border.color: root.ringColor
    radius: root.shapeRadius <= 0 ? 0 : root.shapeRadius + root.offset + root.thickness
    antialiasing: true
    z: 1000
}
