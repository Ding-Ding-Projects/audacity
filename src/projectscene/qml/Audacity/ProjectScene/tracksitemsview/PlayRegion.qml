/*
* Audacity: A Digital Audio Editor
*/

import QtQuick

import Audacity.ProjectScene

import Audacity.M3

Rectangle {
    id: root

    required property TimelineContext context

    property alias start: playRegionModel.start
    property alias end: playRegionModel.end
    property alias active: playRegionModel.active

    y: 0
    height: parent.height / 2

    color: M3.color.primary
    // No dedicated Material 3 token exists yet for a persistent selection-region
    // overlay, so the closest state layer levels on the same primary role stand
    // in: the region reads more strongly while active than while merely shown.
    opacity: active ? M3.stateLayer.dragged : M3.stateLayer.hover

    function updatePosition() {
        let newX = context.timeToPosition(start)
        x = newX > 0 ? newX : 0
        width = context.timeToPosition(end) - x
    }

    onStartChanged: updatePosition()
    onEndChanged: updatePosition()

    Component.onCompleted: {
        playRegionModel.init()
    }

    Connections {
        target: context

        function onFrameStartTimeChanged() {
            updatePosition()
        }

        function onFrameEndTimeChanged() {
            updatePosition()
        }
    }

    PlayRegionModel {
        id: playRegionModel
    }

    MouseArea {
        anchors.verticalCenter: parent.verticalCenter

        anchors.left: mouseAreaResizeLeft.right
        anchors.right: mouseAreaResizeRight.left

        width: 6
        height: parent.height

        hoverEnabled: true
        cursorShape: Qt.OpenHandCursor
        acceptedButtons: Qt.NoButton
    }

    MouseArea {
        id: mouseAreaResizeLeft

        anchors.verticalCenter: parent.verticalCenter

        x: -width / 2
        width: 6
        height: parent.height

        hoverEnabled: true
        cursorShape: Qt.SizeHorCursor
        acceptedButtons: Qt.NoButton
    }

    MouseArea {
        id: mouseAreaResizeRight

        anchors.verticalCenter: parent.verticalCenter

        x: parent.width - width / 2
        width: 6
        height: parent.height

        hoverEnabled: true
        cursorShape: Qt.SizeHorCursor
        acceptedButtons: Qt.NoButton
    }
}
