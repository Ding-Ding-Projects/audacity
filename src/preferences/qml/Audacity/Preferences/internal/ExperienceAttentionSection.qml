/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.UiComponents
import Audacity.M3

BaseSection {
    id: root

    property var settingsModel: null

    title: qsTrc("preferences", "Attention support")

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodyMedium
        text: qsTrc("preferences", "Each of these works on its own, and they can be combined. They are off until you turn them on, and they never change what an action does.")
    }

    Column {
        width: parent.width
        spacing: 12

        M3Switch {
            text: qsTrc("preferences", "Focus")
            accessibleName: qsTrc("preferences", "Focus. Dims the edges of the window so the work surface stands out")
            checked: root.settingsModel ? root.settingsModel.focusMode : false

            navigation.panel: root.navigation
            navigation.name: "FocusMode"
            navigation.row: 1

            onToggled: function (checked) {
                root.settingsModel.focusMode = checked
            }
        }

        M3Switch {
            text: qsTrc("preferences", "Low stimulation")
            accessibleName: qsTrc("preferences", "Low stimulation. Turns off decorative motion and uses a calmer palette")
            checked: root.settingsModel ? root.settingsModel.lowStimulationMode : false

            navigation.panel: root.navigation
            navigation.name: "LowStimulationMode"
            navigation.row: 2

            onToggled: function (checked) {
                root.settingsModel.lowStimulationMode = checked
            }
        }

        M3Switch {
            text: qsTrc("preferences", "Time awareness")
            accessibleName: qsTrc("preferences", "Time awareness. Shows the clock and how long this session has been running")
            checked: root.settingsModel ? root.settingsModel.timeAwarenessMode : false

            navigation.panel: root.navigation
            navigation.name: "TimeAwarenessMode"
            navigation.row: 3

            onToggled: function (checked) {
                root.settingsModel.timeAwarenessMode = checked
            }
        }

        M3Switch {
            text: qsTrc("preferences", "One thing at a time")
            accessibleName: qsTrc("preferences", "One thing at a time. Pushes everything except the current task into the background")
            checked: root.settingsModel ? root.settingsModel.oneThingAtATimeMode : false

            navigation.panel: root.navigation
            navigation.name: "OneThingAtATimeMode"
            navigation.row: 4

            onToggled: function (checked) {
                root.settingsModel.oneThingAtATimeMode = checked
            }
        }

        M3Switch {
            text: qsTrc("preferences", "Momentum")
            accessibleName: qsTrc("preferences", "Momentum. Acknowledges a finished action quietly")
            checked: root.settingsModel ? root.settingsModel.momentumMode : false

            navigation.panel: root.navigation
            navigation.name: "MomentumMode"
            navigation.row: 5

            onToggled: function (checked) {
                root.settingsModel.momentumMode = checked
            }
        }
    }

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodySmall
        text: qsTrc("preferences", "Low stimulation adds to the reduced motion setting in Appearance and never switches it off again on its own.")
    }
}
