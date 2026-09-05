/*
* Audacity: A Digital Audio Editor
*
* M3SnackbarHost
*
* Holds the snackbar queue for a window. Anchor one host near the bottom
* leading corner of the application shell and call show() from anywhere.
*
* API:
*     show(text, actionText, duration), dismissCurrent(), actionTriggered(text)
*/
pragma ComponentBehavior: Bound

import QtQuick

import Audacity.M3

Item {
    id: root

    property var queue: []
    property var current: null

    signal actionTriggered(string message)

    implicitWidth: snackbar.width
    implicitHeight: snackbar.height

    function show(text, actionText, duration) {
        var entry = {
            "text": text,
            "actionText": actionText === undefined ? "" : actionText,
            "duration": duration === undefined ? 4000 : duration
        }
        var next = root.queue.slice()
        next.push(entry)
        root.queue = next
        if (root.current === null) {
            root.advance()
        }
    }

    function advance() {
        if (root.queue.length === 0) {
            root.current = null
            return
        }
        var next = root.queue.slice()
        root.current = next.shift()
        root.queue = next
    }

    function dismissCurrent() {
        root.advance()
    }

    M3Snackbar {
        id: snackbar

        visible: root.current !== null
        opacity: root.current !== null ? 1.0 : 0.0
        y: root.current !== null ? 0 : M3.motion.travel(24)

        text: root.current !== null ? root.current.text : ""
        actionText: root.current !== null ? root.current.actionText : ""
        duration: root.current !== null ? root.current.duration : 0

        Behavior on opacity {
            NumberAnimation {
                duration: M3.motion.medium2
                easing: M3.motion.emphasized
            }
        }

        Behavior on y {
            NumberAnimation {
                duration: M3.motion.medium2
                easing: M3.motion.emphasizedDecelerate
            }
        }

        onActionTriggered: root.actionTriggered(snackbar.text)
        onDismissed: root.advance()
    }
}
