import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Rectangle {
    id: root

    property var mouseArea: null
    property NavigationControl navigationCtrl: null

    color: M3.color.surface
    radius: M3.shape.extraSmall
    border.width: 1
    border.color: M3.color.outlineVariant

    NavigationFocusBorder {
        navigationCtrl: root.navigationCtrl
    }

    states: [
        State {
            name: "HOVERED"
            when: mouseArea.containsMouse && !mouseArea.pressed

            PropertyChanges {
                target: root
                color: M3.color.secondaryContainer
                opacity: 0.4
            }
        },
        State {
            name: "PRESSED"
            when: mouseArea.pressed

            PropertyChanges {
                target: root
                color: M3.color.secondaryContainer
                opacity: 0.7
            }
        }
    ]
}
