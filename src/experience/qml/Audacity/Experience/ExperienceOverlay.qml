/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Experience

// Everything the companion features draw on top of the window: the toast
// stack, the notification centre, the attention support layers and the super
// confirmation gate.
//
// The layer never takes pointer input unless something is actually open, so
// the work surface underneath keeps behaving normally.
Item {
    id: root

    property alias settings: settingsModel

    signal regexBuilderRequested(var searchBar)

    ExperienceSettingsModel {
        id: settingsModel

        Component.onCompleted: settingsModel.load()
    }

    Component.onCompleted: {
        ExperienceBridge.overlay = root
        dimSumSurprise.offer()
    }

    function confirmDestructive(actionName, dataSummary, recoveryNote, invoker, onConfirmed) {
        superConfirmation.pendingCallback = onConfirmed || null
        superConfirmation.open(actionName, dataSummary, recoveryNote, invoker)
        return true
    }

    function openNotificationCentre() {
        notificationCentre.open()
    }

    // Focus mode dims the edges of the window, where the toolbars and the
    // side panels live, and leaves the work surface at full strength.
    Item {
        id: focusDim

        anchors.fill: parent
        visible: settingsModel.focusMode || settingsModel.oneThingAtATimeMode

        readonly property real bandWidth: settingsModel.oneThingAtATimeMode ? 320 : 160
        readonly property real bandHeight: settingsModel.oneThingAtATimeMode ? 140 : 90
        readonly property real dim: settingsModel.oneThingAtATimeMode ? 0.5 : 0.3

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: focusDim.bandHeight
            color: M3.color.scrim
            opacity: focusDim.dim
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: focusDim.bandHeight
            color: M3.color.scrim
            opacity: focusDim.dim
        }

        Rectangle {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: focusDim.bandHeight
            anchors.bottomMargin: focusDim.bandHeight
            width: focusDim.bandWidth
            color: M3.color.scrim
            opacity: focusDim.dim
        }

        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.topMargin: focusDim.bandHeight
            anchors.bottomMargin: focusDim.bandHeight
            width: focusDim.bandWidth
            color: M3.color.scrim
            opacity: focusDim.dim
        }
    }

    // Time awareness: the clock and how long this session has been running.
    M3Card {
        id: timeChip

        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.margins: 16

        visible: settingsModel.timeAwarenessMode
        variant: "filled"
        padding: 10
        width: timeRow.implicitWidth + padding * 2
        implicitHeight: timeRow.implicitHeight + padding * 2

        accessibleName: qsTrc("experience", "Clock and session length")

        property date startedAt: new Date()
        property string clockText: ""
        property string sessionText: ""

        function refresh() {
            var now = new Date()
            timeChip.clockText = Qt.formatTime(now, Qt.locale().timeFormat(Locale.ShortFormat))
            var seconds = Math.max(0, Math.floor((now.getTime() - timeChip.startedAt.getTime()) / 1000))
            var hours = Math.floor(seconds / 3600)
            var minutes = Math.floor((seconds % 3600) / 60)
            timeChip.sessionText = hours + "h " + minutes + "m"
        }

        Timer {
            interval: 1000
            repeat: true
            running: timeChip.visible
            triggeredOnStart: true
            onTriggered: timeChip.refresh()
        }

        Row {
            id: timeRow

            spacing: 12

            StyledTextLabel {
                text: timeChip.clockText
                font: M3.typography.titleMedium
            }

            StyledTextLabel {
                text: qsTrc("experience", "Session %1").arg(timeChip.sessionText)
                font: M3.typography.bodyMedium
                color: M3.color.onSurfaceVariant
            }
        }
    }

    // Momentum: a quiet acknowledgement that something finished. No streak,
    // no score, no comparison with anybody.
    M3Card {
        id: momentum

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 32

        visible: settingsModel.momentumMode && momentum.opacity > 0
        opacity: 0
        variant: "filled"
        padding: 12
        width: momentumLabel.implicitWidth + padding * 2
        implicitHeight: momentumLabel.implicitHeight + padding * 2

        property string message: ""

        function acknowledge(text) {
            if (!settingsModel.momentumMode) {
                return
            }
            momentum.message = text
            momentum.opacity = 1
            momentumTimer.restart()
        }

        StyledTextLabel {
            id: momentumLabel

            text: momentum.message
            font: M3.typography.titleSmall
        }

        Behavior on opacity {
            enabled: !M3.motion.reducedMotion

            NumberAnimation {
                duration: M3.motion.medium2
                easing: M3.motion.standard
            }
        }

        Timer {
            id: momentumTimer

            interval: 2400
            onTriggered: momentum.opacity = 0
        }
    }

    DimSumSurpriseCard {
        id: dimSumSurprise

        anchors.fill: parent
    }

    NotificationHost {
        id: notificationHost

        anchors.fill: parent
    }

    NotificationCentre {
        id: notificationCentre

        anchors.fill: parent

        onRegexBuilderRequested: function (searchBar) {
            root.regexBuilderRequested(searchBar)
        }
    }

    SuperConfirmationDialog {
        id: superConfirmation

        anchors.fill: parent

        property var pendingCallback: null

        onConfirmed: {
            if (superConfirmation.pendingCallback) {
                superConfirmation.pendingCallback()
                superConfirmation.pendingCallback = null
            }
            momentum.acknowledge(qsTrc("experience", "Finished."))
        }

        onCancelled: superConfirmation.pendingCallback = null
    }
}
