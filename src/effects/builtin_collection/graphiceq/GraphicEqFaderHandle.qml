import QtQuick 2.15
import Muse.Ui
import Audacity.M3

Rectangle {
    width: 16
    height: 32
    radius: 2
    color: M3.color.surface
    border.color: ui.theme.extra["graphic_eq_fader_handle_color"]
    border.width: 1

    Rectangle {
        id: fingerGrip // That horizontal dent in the middle of the fader handle

        width: 10
        height: 1
        color: M3.color.onSurface
        y: 16
        anchors.horizontalCenter: parent.horizontalCenter
    }
}
