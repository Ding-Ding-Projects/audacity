/*
* Audacity: A Digital Audio Editor
*/
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

FlatButton {
    id: root

    property bool isDown: true

    property real topRightRadius: M3.shape.extraSmall
    property real bottomRightRadius: M3.shape.extraSmall

    icon: isDown ? IconCode.SMALL_ARROW_DOWN : IconCode.SMALL_ARROW_UP
    iconColor: M3.color.onSurfaceVariant

    backgroundItem: RoundedRectangle {
        id: background

        topRightRadius: root.topRightRadius
        bottomRightRadius: root.bottomRightRadius

        color: M3.color.surfaceContainerHighest

        NavigationFocusBorder {
            navigationCtrl: root.navigation
            drawOutsideParent: !root.drawFocusBorderInsideRect
        }

        states: [
            State {
                name: "PRESSED"
                when: root.mouseArea.pressed

                PropertyChanges {
                    target: background
                    opacity: 1 - M3.stateLayer.pressed
                }
            },
            State {
                name: "HOVERED"
                when: root.mouseArea.containsMouse && !root.mouseArea.pressed

                PropertyChanges {
                    target: background
                    opacity: 1 - M3.stateLayer.hover
                }
            }
        ]
    }

    mouseArea.onPressAndHold: {
        continuousTimer.running = true
    }
    mouseArea.onReleased: {
        continuousTimer.running = false
    }

    Timer {
        id: continuousTimer

        interval: 100
        repeat: true

        onTriggered: {
            root.clicked(null)
        }
    }
}
