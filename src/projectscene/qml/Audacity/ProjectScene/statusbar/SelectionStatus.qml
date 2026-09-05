import QtQuick

import Muse.UiComponents

import Audacity.ProjectScene
import Audacity.UiComponents 1.0

import Audacity.M3

Row {
    id: root

    property NavigationPanel navigationPanel: null

    spacing: 6

    SelectionStatusModel {
        id: selectionModel
    }

    Component.onCompleted: {
        selectionModel.init()
    }

    StyledTextLabel {
        id: titleLabel

        anchors.verticalCenter: parent.verticalCenter

        text: qsTrc("projectscene", "Selection")
        font: M3.typography.labelMedium
        color: M3.color.onSurfaceVariant

        enabled: selectionModel.isEnabled
        opacity: enabled ? 1.0 : M3.stateLayer.disabledContent
    }

    // The selection start and end are two Material 3 outlined fields rather
    // than the shared start and end control, which carries no colour API.
    Row {
        id: startEndTimeCode

        readonly property int navigationRow: startTimecode.navigation.row
        readonly property int navigationColumnEnd: endTimecode.navigationColumnEnd

        anchors.verticalCenter: parent.verticalCenter

        spacing: 1

        Timecode {
            id: startTimecode

            backgroundLeftRadius: M3.shape.small
            backgroundColor: M3.color.surfaceContainerHighest
            textColor: M3.color.onSurface
            border.width: 1
            border.color: M3.color.outline

            value: selectionModel.startTime

            mode: TimecodeModeSelector.TimePoint
            sampleRate: selectionModel.sampleRate
            tempo: selectionModel.tempo
            upperTimeSignature: selectionModel.upperTimeSignature
            lowerTimeSignature: selectionModel.lowerTimeSignature

            currentFormat: selectionModel.currentFormat

            showMenu: false

            enabled: selectionModel.isEnabled

            navigation.panel: root.navigationPanel
            navigation.row: 1
            navigation.column: 1

            accessibleName: qsTrc("projectscene", "Selection start")

            onValueChangeRequested: function (newValue) {
                selectionModel.startTime = newValue
            }
        }

        Timecode {
            id: endTimecode

            backgroundLeftRadius: 0
            backgroundColor: M3.color.surfaceContainerHighest
            textColor: M3.color.onSurface
            border.width: 1
            border.color: M3.color.outline

            value: selectionModel.endTime

            mode: TimecodeModeSelector.TimePoint
            sampleRate: selectionModel.sampleRate
            tempo: selectionModel.tempo
            upperTimeSignature: selectionModel.upperTimeSignature
            lowerTimeSignature: selectionModel.lowerTimeSignature

            currentFormat: selectionModel.currentFormat

            enabled: selectionModel.isEnabled

            navigation.panel: root.navigationPanel
            navigation.row: startTimecode.navigation.row
            navigation.column: startTimecode.navigationColumnEnd + 1

            accessibleName: qsTrc("projectscene", "Selection end")

            onValueChangeRequested: function (newValue) {
                selectionModel.endTime = newValue
            }

            onCurrentFormatChanged: function () {
                selectionModel.currentFormat = endTimecode.currentFormat
            }
        }
    }

    StyledTextLabel {
        id: durationLabel

        anchors.verticalCenter: parent.verticalCenter

        text: qsTrc("projectscene", "Duration")
        font: M3.typography.labelMedium
        color: M3.color.onSurfaceVariant

        enabled: selectionModel.isEnabled
        opacity: enabled ? 1.0 : M3.stateLayer.disabledContent
    }

    Timecode {
        id: durationTimecode

        backgroundLeftRadius: M3.shape.small
        backgroundColor: M3.color.surfaceContainerHighest
        textColor: M3.color.onSurface
        border.width: 1
        border.color: M3.color.outline

        value: selectionModel.endTime - selectionModel.startTime
        mode: TimecodeModeSelector.Duration

        sampleRate: selectionModel.sampleRate
        tempo: selectionModel.tempo
        upperTimeSignature: selectionModel.upperTimeSignature
        lowerTimeSignature: selectionModel.lowerTimeSignature

        currentFormat: selectionModel.durationFormat

        enabled: selectionModel.isEnabled

        navigation.panel: root.navigationPanel
        navigation.row: startEndTimeCode.navigationRow
        navigation.column: startEndTimeCode.navigationColumnEnd + 1

        accessibleName: durationLabel.text

        onValueChangeRequested: function (newValue) {
            selectionModel.endTime = selectionModel.startTime + newValue
        }

        onCurrentFormatChanged: function () {
            selectionModel.durationFormat = currentFormat
        }
    }
}
