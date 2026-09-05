/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

// Adds or edits one row of the scheduled settings table.
M3BottomSheet {
    id: root

    property var scheduleModel: null

    property string entryId: ""
    property int hour: 9
    property int minute: 0
    property int weekdayMask: 0b1111111
    property string settingKey: "theme"
    property string settingValue: "dark"

    readonly property var currentSetting: {
        if (!root.scheduleModel) {
            return null
        }
        var settings = root.scheduleModel.availableSettings()
        for (var i = 0; i < settings.length; ++i) {
            if (settings[i].key === root.settingKey) {
                return settings[i]
            }
        }
        return null
    }

    headline: root.entryId === "" ? qsTrc("experience", "New scheduled change") : qsTrc("experience", "Edit scheduled change")
    sheetHeight: 520

    function edit(row) {
        root.entryId = row.id
        root.hour = row.hour
        root.minute = row.minute
        root.weekdayMask = row.weekdayMask
        root.settingKey = row.key
        root.settingValue = row.value
        root.open()
    }

    Column {
        anchors.fill: parent
        spacing: 20

        Column {
            width: parent.width
            spacing: 8

            StyledTextLabel {
                horizontalAlignment: Text.AlignLeft
                font: M3.typography.titleSmall
                text: qsTrc("experience", "Time of day")
            }

            M3TimePicker {
                id: timePicker

                hours: root.hour
                minutes: root.minute
                use24Hour: true

                onTimeChanged: function (hours, minutes) {
                    root.hour = hours
                    root.minute = minutes
                }
            }
        }

        Column {
            width: parent.width
            spacing: 8

            StyledTextLabel {
                horizontalAlignment: Text.AlignLeft
                font: M3.typography.titleSmall
                text: qsTrc("experience", "Days")
            }

            Row {
                spacing: 8

                Repeater {
                    model: [
                        {
                            bit: 0,
                            name: qsTrc("experience", "Mon")
                        },
                        {
                            bit: 1,
                            name: qsTrc("experience", "Tue")
                        },
                        {
                            bit: 2,
                            name: qsTrc("experience", "Wed")
                        },
                        {
                            bit: 3,
                            name: qsTrc("experience", "Thu")
                        },
                        {
                            bit: 4,
                            name: qsTrc("experience", "Fri")
                        },
                        {
                            bit: 5,
                            name: qsTrc("experience", "Sat")
                        },
                        {
                            bit: 6,
                            name: qsTrc("experience", "Sun")
                        }
                    ]

                    delegate: M3Chip {
                        id: dayChip

                        required property var modelData

                        // The chip owns its own checked state once it is
                        // pressed, so the mask is mirrored explicitly rather
                        // than through a binding that the first press removes.
                        readonly property bool inMask: (root.weekdayMask & (1 << dayChip.modelData.bit)) !== 0

                        variant: "filter"
                        text: dayChip.modelData.name
                        accessibleName: dayChip.modelData.name

                        onInMaskChanged: dayChip.checked = dayChip.inMask
                        Component.onCompleted: dayChip.checked = dayChip.inMask

                        onToggled: function (checked) {
                            if (checked) {
                                root.weekdayMask |= (1 << dayChip.modelData.bit)
                            } else {
                                root.weekdayMask &= ~(1 << dayChip.modelData.bit)
                            }
                        }
                    }
                }
            }
        }

        Column {
            width: parent.width
            spacing: 8

            StyledTextLabel {
                horizontalAlignment: Text.AlignLeft
                font: M3.typography.titleSmall
                text: qsTrc("experience", "Change")
            }

            M3Dropdown {
                id: settingDropdown

                width: 280
                model: root.scheduleModel ? root.scheduleModel.availableSettings() : []
                textRole: "title"
                valueRole: "key"
                label: qsTrc("experience", "Setting")

                onActivated: function (index, value) {
                    root.settingKey = value
                    var setting = root.currentSetting
                    if (setting && setting.choices.length > 0) {
                        root.settingValue = setting.choices[0].value
                    }
                }
            }

            M3Dropdown {
                id: valueDropdown

                width: 280
                model: root.currentSetting ? root.currentSetting.choices : []
                textRole: "title"
                valueRole: "value"
                label: qsTrc("experience", "Value")

                onActivated: function (index, value) {
                    root.settingValue = value
                }
            }
        }

        Row {
            anchors.right: parent.right
            spacing: 12

            M3Button {
                text: qsTrc("experience", "Cancel")
                variant: "text"

                onClicked: root.close()
            }

            M3Button {
                text: qsTrc("experience", "Save")
                variant: "filled"
                enabled: root.weekdayMask !== 0 && root.settingValue !== ""

                onClicked: {
                    root.scheduleModel.save({
                        "id": root.entryId,
                        "enabled": true,
                        "hour": root.hour,
                        "minute": root.minute,
                        "weekdayMask": root.weekdayMask,
                        "key": root.settingKey,
                        "value": root.settingValue
                    })
                    root.close()
                }
            }
        }
    }
}
