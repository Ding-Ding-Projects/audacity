/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

// The gate in front of an action that destroys data.
//
// It asks for three separate, deliberate movements: two key switches that sit
// apart from each other, then a slider dragged the whole way across. An
// emergency exit is available at every stage, and Escape cancels.
FocusScope {
    id: root

    // The exact action, for example "Remove 3 tracks".
    property string actionName: ""
    // What is affected, in the reader's own terms.
    property string dataSummary: ""
    // Extra text shown under the summary, for example how to get the data back.
    property string recoveryNote: ""

    property bool opened: false

    readonly property bool keysTurned: firstKey.checked && secondKey.checked
    readonly property bool released: slider.value >= slider.to

    signal confirmed
    signal cancelled

    // The control the request came from. The focus goes back to it on close.
    property var invoker: null

    property int stage: 0 // 0 keys, 1 release, 2 running, 3 done

    function open(name, summary, note, invokingControl) {
        root.actionName = name
        root.dataSummary = summary
        root.recoveryNote = note || ""
        root.invoker = invokingControl || null
        root.stage = 0
        firstKey.checked = false
        secondKey.checked = false
        slider.value = 0
        root.opened = true
        root.forceActiveFocus()
        firstKey.navigation.requestActive()
    }

    function close() {
        root.opened = false
        root.stage = 0
        if (root.invoker && typeof root.invoker.forceActiveFocus === "function") {
            root.invoker.forceActiveFocus()
        }
    }

    function cancel() {
        root.close()
        root.cancelled()
    }

    visible: root.opened
    anchors.fill: parent

    NavigationSection {
        id: navSection

        name: "SuperConfirmation"
        enabled: root.opened
        order: 1000
    }

    NavigationPanel {
        id: navPanel

        name: "SuperConfirmationPanel"
        section: navSection
        direction: NavigationPanel.Vertical
        order: 1
    }

    Keys.onEscapePressed: root.cancel()
    Keys.onBackPressed: root.cancel()

    Rectangle {
        anchors.fill: parent
        color: M3.color.scrim
        opacity: 0.5

        MouseArea {
            anchors.fill: parent
            // The scrim swallows clicks so nothing behind the gate can be
            // reached by accident, but clicking it does not cancel: leaving
            // has to be as deliberate as continuing.
            hoverEnabled: true
        }
    }

    M3Card {
        id: card

        anchors.centerIn: parent
        width: Math.min(560, root.width - 48)
        implicitHeight: content.implicitHeight + card.padding * 2

        variant: "elevated"
        padding: 24

        Column {
            id: content

            width: parent.width
            spacing: 20

            Column {
                width: parent.width
                spacing: 8

                StyledTextLabel {
                    width: parent.width
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.WordWrap
                    font: M3.typography.headlineSmall
                    text: qsTrc("experience", "Confirm: %1").arg(root.actionName)
                }

                StyledTextLabel {
                    width: parent.width
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.WordWrap
                    font: M3.typography.bodyMedium
                    color: M3.color.onSurfaceVariant
                    text: root.dataSummary
                }

                StyledTextLabel {
                    width: parent.width
                    horizontalAlignment: Text.AlignLeft
                    wrapMode: Text.WordWrap
                    visible: root.recoveryNote !== ""
                    font: M3.typography.bodySmall
                    color: M3.color.onSurfaceVariant
                    text: root.recoveryNote
                }
            }

            M3Divider {
                width: parent.width
            }

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                font: M3.typography.titleSmall
                text: qsTrc("experience", "Step 1. Turn both keys")
            }

            Item {
                width: parent.width
                height: Math.max(firstKey.height, secondKey.height)

                M3Switch {
                    id: firstKey

                    anchors.left: parent.left
                    text: qsTrc("experience", "Left key")
                    accessibleName: qsTrc("experience", "Left key, step 1 of 3")
                    enabled: root.stage < 2

                    navigation.panel: navPanel
                    navigation.row: 1

                    onToggled: function (checked) {
                        firstKey.checked = checked
                    }
                }

                M3Switch {
                    id: secondKey

                    anchors.right: parent.right
                    text: qsTrc("experience", "Right key")
                    accessibleName: qsTrc("experience", "Right key, step 1 of 3")
                    enabled: root.stage < 2

                    navigation.panel: navPanel
                    navigation.row: 2

                    onToggled: function (checked) {
                        secondKey.checked = checked
                    }
                }
            }

            Column {
                width: parent.width
                spacing: 8
                opacity: root.keysTurned ? 1.0 : 0.4

                StyledTextLabel {
                    width: parent.width
                    horizontalAlignment: Text.AlignLeft
                    font: M3.typography.titleSmall
                    text: qsTrc("experience", "Step 2. Drag all the way to the right")
                }

                M3Slider {
                    id: slider

                    width: parent.width
                    from: 0
                    to: 100
                    stepSize: 1
                    enabled: root.keysTurned && root.stage < 2
                    accessibleName: qsTrc("experience", "Release control, step 2 of 3. Drag to the end to continue")
                    valueText: Math.round(slider.value) + "%"

                    navigation.panel: navPanel
                    navigation.row: 3

                    onMoved: {
                        if (root.released && root.stage === 0) {
                            root.stage = 2
                            runProgress.restart()
                        }
                    }
                }
            }

            Column {
                width: parent.width
                spacing: 8
                visible: root.stage >= 2

                StyledTextLabel {
                    width: parent.width
                    horizontalAlignment: Text.AlignLeft
                    font: M3.typography.titleSmall
                    text: root.stage === 3 ? qsTrc("experience", "Step 3. Done") : qsTrc("experience", "Step 3. Carrying it out")
                }

                M3LinearProgress {
                    id: progress

                    width: parent.width
                    from: 0
                    to: 100
                    value: 0
                    wavy: root.stage === 2 && !M3.motion.reducedMotion
                }

                StyledTextLabel {
                    id: completion

                    width: parent.width
                    horizontalAlignment: Text.AlignLeft
                    visible: root.stage === 3
                    color: M3.color.tertiary
                    font: M3.typography.titleMedium
                    text: qsTrc("experience", "%1 is complete.").arg(root.actionName)
                    scale: 1.0
                }
            }

            Row {
                anchors.right: parent.right
                spacing: 12

                M3Button {
                    // Always available, at every stage.
                    text: qsTrc("experience", "Emergency exit")
                    variant: "text"
                    accessibleName: qsTrc("experience", "Emergency exit, cancel %1").arg(root.actionName)

                    navigation.panel: navPanel
                    navigation.row: 4

                    onClicked: root.cancel()
                }

                M3Button {
                    text: qsTrc("experience", "Close")
                    variant: "filled"
                    visible: root.stage === 3

                    navigation.panel: navPanel
                    navigation.row: 5

                    onClicked: root.close()
                }
            }
        }
    }

    // The dramatic run. It never blocks: the reader can still press the
    // emergency exit while it plays. Under reduced motion it takes no time at
    // all and lands straight on the finished state.
    SequentialAnimation {
        id: runProgress

        NumberAnimation {
            target: progress
            property: "value"
            from: 0
            to: 100
            duration: M3.motion.reducedMotion ? 0 : 1400
            easing: M3.motion.emphasized
        }

        ScriptAction {
            script: {
                root.stage = 3
                root.confirmed()
                completionPulse.restart()
            }
        }
    }

    // A distinct finish: the completion line settles rather than sweeps.
    SequentialAnimation {
        id: completionPulse

        NumberAnimation {
            target: completion
            property: "scale"
            from: M3.motion.reducedMotion ? 1.0 : 0.9
            to: 1.0
            duration: M3.motion.reducedMotion ? 0 : M3.motion.medium2
            easing: M3.motion.emphasizedDecelerate
        }
    }
}
