/*
* Audacity: A Digital Audio Editor
*/
import QtQuick

import Muse.UiComponents

import Audacity.ProjectScene
import Audacity.M3

TrackItem {
    id: root

    signal addLabelToSelectionRequested

    extraControlsComponent: Component {
        M3Button {
            width: parent.width
            height: M3.density.apply(32)

            variant: "outlined"

            text: qsTrc("projectscene", "Add label")
            accessibleName: qsTrc("projectscene", "Add label")

            opacity: root.collapsed ? 0 : 1
            visible: opacity !== 0
            Behavior on opacity {
                OpacityAnimator {
                    duration: M3.motion.short4
                    easing: M3.motion.standard
                }
            }

            navigation.panel: root.headerNavigationPanel
            navigation.order: root.extraControlsNavigationStart
            navigation.enabled: !root.collapsed

            onClicked: {
                root.addLabelToSelectionRequested()
            }
        }
    }
}
