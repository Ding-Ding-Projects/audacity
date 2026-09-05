/*
* Audacity: A Digital Audio Editor
*
* M3TimePicker
*
* The Material 3 time input variant: two large numeric fields for the hour and
* the minute with a segmented button for AM and PM in twelve hour mode. The
* input variant is used rather than the dial because it is faster with a
* keyboard and is fully reachable through muse navigation.
*
* API:
*     hours (0 to 23), minutes, use24Hour, timeChanged(hours, minutes),
*     navigationPanel
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

M3Surface {
    id: root

    property int hours: 0
    property int minutes: 0
    property bool use24Hour: true

    property NavigationPanel navigationPanel: null

    signal timeChanged(int hours, int minutes)

    level: 3
    radius: M3.shape.extraLarge

    implicitWidth: 328
    implicitHeight: 200

    readonly property bool afternoon: root.hours >= 12

    readonly property int displayHours: {
        if (root.use24Hour) {
            return root.hours
        }
        var value = root.hours % 12
        return value === 0 ? 12 : value
    }

    function pad(value) {
        return value < 10 ? "0" + value : String(value)
    }

    function setHours(value) {
        var bounded = root.use24Hour ? Math.max(0, Math.min(23, value)) : Math.max(1, Math.min(12, value))
        if (!root.use24Hour) {
            bounded = (bounded % 12) + (root.afternoon ? 12 : 0)
        }
        root.hours = bounded
        root.timeChanged(root.hours, root.minutes)
    }

    function setMinutes(value) {
        root.minutes = Math.max(0, Math.min(59, value))
        root.timeChanged(root.hours, root.minutes)
    }

    function setAfternoon(value) {
        var base = root.hours % 12
        root.hours = base + (value ? 12 : 0)
        root.timeChanged(root.hours, root.minutes)
    }

    Row {
        anchors.centerIn: parent
        spacing: 12

        M3TextField {
            id: hourField

            width: 96
            variant: "filled"
            label: "Hour"
            currentText: root.pad(root.displayHours)
            navigation.panel: root.navigationPanel
            navigation.column: 0

            onTextEditingFinished: function (text) {
                var value = parseInt(text, 10)
                if (!isNaN(value)) {
                    root.setHours(value)
                }
            }
        }

        StyledTextLabel {
            anchors.verticalCenter: parent.verticalCenter
            text: ":"
            font: M3.typography.displayMedium
            color: M3.color.onSurface
        }

        M3TextField {
            id: minuteField

            width: 96
            variant: "filled"
            label: "Minute"
            currentText: root.pad(root.minutes)
            navigation.panel: root.navigationPanel
            navigation.column: 1

            onTextEditingFinished: function (text) {
                var value = parseInt(text, 10)
                if (!isNaN(value)) {
                    root.setMinutes(value)
                }
            }
        }

        M3SegmentedButton {
            anchors.verticalCenter: parent.verticalCenter
            visible: !root.use24Hour
            model: ["AM", "PM"]
            currentIndex: root.afternoon ? 1 : 0
            navigationPanel: root.navigationPanel
            navigationRowStart: 1

            onActivated: function (index) {
                root.setAfternoon(index === 1)
            }
        }
    }
}
