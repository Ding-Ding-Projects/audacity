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
    // -1 means no end time: the row fires once, at hour:minute, instead of
    // holding a window open.
    property int endHour: -1
    property int endMinute: -1
    property string startDate: ""
    property string endDate: ""
    property int weekdayMask: 0b1111111
    property string settingKey: "theme"
    property string settingValue: "dark"
    // One of "local", "httpsApi", "homeAssistant".
    property string source: "local"
    property string apiUrl: ""
    property string haBaseUrl: ""
    property string haEntityId: ""

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
        root.endHour = row.endHour !== undefined ? row.endHour : -1
        root.endMinute = row.endMinute !== undefined ? row.endMinute : -1
        root.startDate = row.startDate || ""
        root.endDate = row.endDate || ""
        root.weekdayMask = row.weekdayMask
        root.settingKey = row.key
        root.settingValue = row.value
        root.source = row.source || "local"
        root.apiUrl = row.apiUrl || ""
        root.haBaseUrl = row.haBaseUrl || ""
        root.haEntityId = row.haEntityId || ""
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

        Column {
            width: parent.width
            spacing: 8

            M3Switch {
                id: hasEndTimeSwitch

                text: qsTrc("experience", "Hold a window open, and restore the setting when it ends")
                accessibleName: text
                checked: root.endHour >= 0

                onToggled: function (checked) {
                    if (checked) {
                        root.endHour = (root.hour + 1) % 24
                        root.endMinute = root.minute
                    } else {
                        root.endHour = -1
                        root.endMinute = -1
                    }
                }
            }

            M3TimePicker {
                id: endTimePicker

                visible: hasEndTimeSwitch.checked
                hours: root.endHour >= 0 ? root.endHour : 10
                minutes: root.endMinute >= 0 ? root.endMinute : 0
                use24Hour: true

                onTimeChanged: function (hours, minutes) {
                    root.endHour = hours
                    root.endMinute = minutes
                }
            }
        }

        Column {
            width: parent.width
            spacing: 8

            StyledTextLabel {
                horizontalAlignment: Text.AlignLeft
                font: M3.typography.titleSmall
                text: qsTrc("experience", "Starts from")
            }

            Row {
                spacing: 12

                M3TextField {
                    id: startDateField

                    width: 140
                    currentText: root.startDate
                    placeholder: qsTrc("experience", "Any day, no start")

                    onTextEdited: function (text) {
                        root.startDate = text
                    }
                }

                M3TextField {
                    id: endDateField

                    width: 140
                    currentText: root.endDate
                    placeholder: qsTrc("experience", "Keeps going, no end")

                    onTextEdited: function (text) {
                        root.endDate = text
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
                text: qsTrc("experience", "Where the value comes from")
            }

            M3Dropdown {
                id: sourceDropdown

                width: 280
                model: [
                    {
                        "title": qsTrc("experience", "This computer"),
                        "value": "local"
                    },
                    {
                        "title": qsTrc("experience", "An HTTPS API"),
                        "value": "httpsApi"
                    },
                    {
                        "title": qsTrc("experience", "A Home Assistant switch"),
                        "value": "homeAssistant"
                    }
                ]
                textRole: "title"
                valueRole: "value"
                label: qsTrc("experience", "Source")
                currentIndex: root.source === "httpsApi" ? 1 : (root.source === "homeAssistant" ? 2 : 0)

                onActivated: function (index, value) {
                    root.source = value
                }
            }

            M3TextField {
                id: apiUrlField

                visible: root.source === "httpsApi"
                width: parent.width
                currentText: root.apiUrl
                placeholder: "https://example.com/settings.json"

                onTextEdited: function (text) {
                    root.apiUrl = text
                }
            }

            M3TextField {
                id: haBaseUrlField

                visible: root.source === "homeAssistant"
                width: parent.width
                currentText: root.haBaseUrl
                placeholder: "https://homeassistant.local:8123"

                onTextEdited: function (text) {
                    root.haBaseUrl = text
                }
            }

            M3TextField {
                id: haEntityField

                visible: root.source === "homeAssistant"
                width: parent.width
                currentText: root.haEntityId
                placeholder: "input_boolean.night_mode"

                onTextEdited: function (text) {
                    root.haEntityId = text
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
                enabled: root.weekdayMask !== 0 && (root.source !== "local" || root.settingValue !== "") && (root.source !== "httpsApi" || root.apiUrl !== "") && (root.source !== "homeAssistant" || (root.haBaseUrl !== "" && root.haEntityId !== ""))

                onClicked: {
                    root.scheduleModel.save({
                        "id": root.entryId,
                        "enabled": true,
                        "hour": root.hour,
                        "minute": root.minute,
                        "endHour": root.endHour,
                        "endMinute": root.endMinute,
                        "startDate": root.startDate,
                        "endDate": root.endDate,
                        "weekdayMask": root.weekdayMask,
                        "key": root.settingKey,
                        "value": root.settingValue,
                        "source": root.source,
                        "apiUrl": root.apiUrl,
                        "haBaseUrl": root.haBaseUrl,
                        "haEntityId": root.haEntityId
                    })
                    root.close()
                }
            }
        }
    }
}
