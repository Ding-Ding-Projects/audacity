/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.UiComponents
import Audacity.M3
import Audacity.Experience

BaseSection {
    id: root

    title: qsTrc("preferences", "Scheduled changes")

    ScheduleListModel {
        id: scheduleListModel

        Component.onCompleted: scheduleListModel.load()
    }

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        wrapMode: Text.WordWrap
        color: M3.color.onSurfaceVariant
        font: M3.typography.bodyMedium
        text: qsTrc("preferences", "Change a setting at a time of day, on the days you choose.")
    }

    StyledTextLabel {
        width: parent.width
        horizontalAlignment: Text.AlignLeft
        visible: scheduleListModel.count === 0
        color: M3.color.onSurfaceVariant
        text: qsTrc("preferences", "Nothing is scheduled.")
    }

    Column {
        width: parent.width
        spacing: 4

        Repeater {
            model: scheduleListModel

            delegate: M3ListItem {
                id: scheduleRow

                required property var model

                width: parent.width

                overline: scheduleRow.model.daysText
                headline: scheduleRow.model.timeText + "  " + scheduleRow.model.settingText
                supportingText: scheduleRow.model.nextFireText
                accessibleName: scheduleRow.model.timeText + ", " + scheduleRow.model.daysText + ", " + scheduleRow.model.settingText

                trailingContent: Row {
                    spacing: 8

                    M3Switch {
                        checked: scheduleRow.model.entryEnabled
                        showIcon: false
                        accessibleName: qsTrc("preferences", "Scheduled change is on")

                        onToggled: function (checked) {
                            scheduleListModel.setEnabled(scheduleRow.model.entryId, checked)
                        }
                    }

                    M3IconButton {
                        icon: IconCode.DELETE_TANK
                        accessibleName: qsTrc("preferences", "Remove this scheduled change")

                        onClicked: scheduleListModel.remove(scheduleRow.model.entryId)
                    }
                }

                onClicked: editSheet.edit(scheduleListModel.row(scheduleRow.model.entryId))
            }
        }
    }

    M3Button {
        text: qsTrc("preferences", "Add a scheduled change")
        variant: "outlined"
        icon: IconCode.PLUS

        navigation.panel: root.navigation
        navigation.name: "AddScheduledChange"
        navigation.row: 1

        onClicked: editSheet.edit(scheduleListModel.row(""))
    }

    ScheduleEditSheet {
        id: editSheet

        parent: Window.contentItem
        scheduleModel: scheduleListModel
    }
}
