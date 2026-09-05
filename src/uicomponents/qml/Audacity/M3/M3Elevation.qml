/*
* Audacity: A Digital Audio Editor
*
* M3Elevation
*
* Draws the Material 3 key and ambient shadows for an elevation level. It uses
* layered rectangles rather than a graphics effect so that it works on every
* backend Audacity ships on.
*
* Usage:
*     M3Elevation {
*         anchors.fill: parent
*         level: 2
*         radius: M3.shape.medium
*     }
*/
pragma ComponentBehavior: Bound

import QtQuick

import Audacity.M3

Item {
    id: root

    // Elevation level, 0 to 5.
    property int level: 0
    property real radius: 0
    property color shadowColor: M3.color.shadow

    visible: root.level > 0
    z: -1

    readonly property real keyBlur: M3.elevation.keyBlur(root.level)
    readonly property real keyOffset: M3.elevation.keyOffset(root.level)
    readonly property real ambientBlur: M3.elevation.ambientBlur(root.level)

    // Ambient shadow: a soft halo on every side.
    Repeater {
        model: root.visible ? Math.max(1, Math.round(root.ambientBlur)) : 0

        Rectangle {
            required property int index

            anchors.centerIn: parent
            width: root.width + index * 2
            height: root.height + index * 2
            radius: root.radius > 0 ? root.radius + index : 0
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(root.shadowColor.r, root.shadowColor.g, root.shadowColor.b, 0.06 * (1.0 - index / Math.max(1, root.ambientBlur)))
            antialiasing: true
        }
    }

    // Key shadow: offset downwards, tighter and darker.
    Repeater {
        model: root.visible ? Math.max(1, Math.round(root.keyBlur)) : 0

        Rectangle {
            required property int index

            anchors.horizontalCenter: parent.horizontalCenter
            y: root.keyOffset - index
            width: root.width + index * 2
            height: root.height + index * 2
            radius: root.radius > 0 ? root.radius + index : 0
            color: "transparent"
            border.width: 1
            border.color: Qt.rgba(root.shadowColor.r, root.shadowColor.g, root.shadowColor.b, 0.10 * (1.0 - index / Math.max(1, root.keyBlur)))
            antialiasing: true
        }
    }
}
