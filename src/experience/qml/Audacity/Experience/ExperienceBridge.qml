/*
 * Audacity: A Digital Audio Editor
 */
pragma Singleton

import QtQuick

// The single point any part of the interface uses to reach the companion
// surfaces. The overlay registers itself here when the window is built.
QtObject {
    id: bridge

    // Set by ExperienceOverlay.
    property var overlay: null

    readonly property bool available: bridge.overlay !== null

    // Opens the super confirmation gate in front of a destructive action.
    //
    //   ExperienceBridge.confirmDestructive(
    //       qsTrc("appshell", "Clear history"),
    //       qsTrc("appshell", "Every undo step for this project is removed."),
    //       qsTrc("appshell", "This cannot be undone."),
    //       theButtonThatAsked,
    //       function () { doTheThing() })
    //
    // Returns false when the overlay is not available, so a call site can fall
    // back to its own confirmation.
    function confirmDestructive(actionName, dataSummary, recoveryNote, invoker, onConfirmed) {
        if (!bridge.available) {
            return false
        }
        return bridge.overlay.confirmDestructive(actionName, dataSummary, recoveryNote, invoker, onConfirmed)
    }

    function openNotificationCentre() {
        if (bridge.available) {
            bridge.overlay.openNotificationCentre()
        }
    }

    // Alias kept for call sites written against the documented name
    // "SuperConfirmation.request(actionName, dataSummary, recoveryNote,
    // invoker, onConfirmed)". Behaves exactly like confirmDestructive above.
    function request(actionName, dataSummary, recoveryNote, invoker, onConfirmed) {
        return bridge.confirmDestructive(actionName, dataSummary, recoveryNote, invoker, onConfirmed)
    }
}
