/*
* Audacity: A Digital Audio Editor
*
* M3Divider
*
* A one pixel outline variant rule. Set orientation to Qt.Vertical for a
* column separator.
*
* Replaces: Muse.UiComponents SeparatorLine.
*
* API:
*     orientation, inset, thickness
*/
import QtQuick

import Audacity.M3

Rectangle {
    id: root

    property int orientation: Qt.Horizontal

    // Leading and trailing inset, as used by list dividers.
    property real inset: 0
    property real thickness: 1

    color: M3.color.outlineVariant

    implicitWidth: root.orientation === Qt.Horizontal ? 0 : root.thickness
    implicitHeight: root.orientation === Qt.Horizontal ? root.thickness : 0

    anchors.leftMargin: root.orientation === Qt.Horizontal ? root.inset : 0
    anchors.rightMargin: root.orientation === Qt.Horizontal ? root.inset : 0
    anchors.topMargin: root.orientation === Qt.Horizontal ? 0 : root.inset
    anchors.bottomMargin: root.orientation === Qt.Horizontal ? 0 : root.inset
}
