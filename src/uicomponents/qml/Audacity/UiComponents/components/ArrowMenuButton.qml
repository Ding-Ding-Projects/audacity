/*
* Audacity: A Digital Audio Editor
*/
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

MenuButton {
    id: root

    property color backgroundColor: M3.color.surfaceContainerHighest
    property Border border: Border {}

    icon: IconCode.SMALL_ARROW_DOWN
    iconColor: M3.color.onSurfaceVariant

    menuAnchorItem: ui.rootItem

    navigation.name: "ArrowMenuButton"

    backgroundItem: RoundedRectangle {
        id: background

        topRightRadius: M3.shape.extraSmall
        bottomRightRadius: M3.shape.extraSmall

        color: root.backgroundColor
        border: root.border

        NavigationFocusBorder {
            navigationCtrl: root.navigation
            drawOutsideParent: !root.drawFocusBorderInsideRect
        }

        states: [
            State {
                name: "PRESSED"
                when: menuBtn.mouseArea.pressed

                PropertyChanges {
                    target: background
                    opacity: 1 - M3.stateLayer.pressed
                }
            },
            State {
                name: "HOVERED"
                when: menuBtn.mouseArea.containsMouse && !menuBtn.mouseArea.pressed

                PropertyChanges {
                    target: background
                    opacity: 1 - M3.stateLayer.hover
                }
            }
        ]
    }
}
