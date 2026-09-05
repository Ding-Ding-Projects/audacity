import QtQuick 2.15

import Muse.Ui 1.0
import Muse.UiComponents

import Audacity.UiComponents 1.0
import Audacity.M3

BaseSection {
    id: root

    title: qsTrc("preferences", "Mono & stereo conversion")

    property bool askBeforeConverting: true

    navigationOrderEnd: root.navigation.order

    Column {
        width: parent.width
        spacing: 24

        M3Switch {
            id: checkbox

            width: parent.width

            text: qsTrc("preferences", "Always convert to mono without prompt")

            checked: !root.askBeforeConverting

            navigation.name: "StereoToMonoBox"
            navigation.panel: root.navigation

            onToggled: {
                root.askBeforeConverting = !root.askBeforeConverting
                checkbox.checked = Qt.binding(function () {
                    return !root.askBeforeConverting
                })
            }
        }
    }
}
