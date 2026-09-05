import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.ProjectScene

import Audacity.M3

Item {
    id: root

    property int icon: IconCode.NONE
    property string text: ""
    property color textColor: M3.color.onSurface
    property color iconColor: M3.color.onSurface

    property alias mouseArea: clipPropertyMouseArea

    signal clicked(var mouse)

    anchors.verticalCenter: parent.verticalCenter
    height: 16
    implicitWidth: content.implicitWidth + 4

    Accessible.role: Accessible.Button
    Accessible.name: root.text

    RowLayout {
        id: content

        anchors.fill: parent
        spacing: 2

        StyledIconLabel {
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: 2
            Layout.preferredWidth: 8
            Layout.preferredHeight: 8
            iconCode: root.icon
            color: root.iconColor
            font.pixelSize: 12
        }

        StyledTextLabel {
            Layout.fillWidth: true
            Layout.rightMargin: 2
            Layout.alignment: Qt.AlignVCenter
            text: root.text
            color: root.textColor
        }
    }

    MouseArea {
        id: clipPropertyMouseArea

        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        hoverEnabled: true

        onClicked: function (mouse) {
            root.clicked(mouse)
        }
    }

    M3StateLayer {
        anchors.fill: parent
        color: root.textColor
        active: root.enabled
        hovered: clipPropertyMouseArea.containsMouse
        pressed: clipPropertyMouseArea.containsPress
    }
}
