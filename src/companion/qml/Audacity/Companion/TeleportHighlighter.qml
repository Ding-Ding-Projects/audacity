/*
* Audacity: A Digital Audio Editor
*
* TeleportHighlighter
*
* The receiving half of a command palette teleport. The palette opens the
* surface and names the control it wants; this item finds that control inside
* the surface at run time, scrolls it into view, gives it keyboard focus and
* pulses a primary coloured highlight over it for 1.2 seconds.
*
* Nothing in the surface has to be prepared for it: the search walks the item
* tree looking for an item whose visible text matches, which is the same text
* the palette indexed.
*
* Under reduced motion the pulse does not animate. The highlight still appears
* and still disappears after the same 1.2 seconds, so the affordance is not
* lost, it simply does not move.
*
* API:
*     root (the item to search), target, highlight(target), cleared()
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui

import Audacity.M3

Item {
    id: highlighter

    //! The item whose children are searched. Defaults to the parent.
    property Item searchRoot: highlighter.parent

    //! The label of the control to find.
    property string target: ""

    //! How long the highlight stays, in milliseconds. Material's own long
    //! emphasis duration doubled, which reads as deliberate without being slow.
    readonly property int holdDuration: 1200

    signal cleared

    anchors.fill: parent
    z: 1000
    visible: found !== null

    property Item found: null

    onTargetChanged: {
        if (highlighter.target !== "") {
            highlighter.highlight(highlighter.target)
        }
    }

    function textOf(item) {
        var words = []
        if (item.text !== undefined && typeof item.text === "string") {
            words.push(item.text)
        }
        if (item.title !== undefined && typeof item.title === "string") {
            words.push(item.title)
        }
        if (item.label !== undefined && typeof item.label === "string") {
            words.push(item.label)
        }
        if (item.headline !== undefined && typeof item.headline === "string") {
            words.push(item.headline)
        }
        return words
    }

    function search(item, needle, depth) {
        if (!Boolean(item) || depth > 12) {
            return null
        }
        var words = highlighter.textOf(item)
        for (var w = 0; w < words.length; ++w) {
            if (words[w] !== "" && words[w].indexOf(needle) !== -1) {
                return item
            }
        }
        var children = item.children
        if (Boolean(children)) {
            for (var i = 0; i < children.length; ++i) {
                var hit = highlighter.search(children[i], needle, depth + 1)
                if (hit !== null) {
                    return hit
                }
            }
        }
        return null
    }

    // Walks up from the found label to the nearest ancestor that is tall
    // enough to be the control itself rather than only its caption.
    function controlOf(item) {
        var candidate = item
        var walker = item
        for (var i = 0; i < 4 && Boolean(walker.parent); ++i) {
            walker = walker.parent
            if (walker.height > candidate.height && walker.height < 200) {
                candidate = walker
            }
        }
        return candidate
    }

    function highlight(needle) {
        highlighter.found = null
        if (needle === "" || !Boolean(highlighter.searchRoot)) {
            return
        }

        var hit = highlighter.search(highlighter.searchRoot, needle, 0)
        if (hit === null) {
            highlighter.cleared()
            return
        }

        highlighter.found = highlighter.controlOf(hit);

        // Scroll the control into view inside whatever Flickable holds it.
        var walker = highlighter.found
        for (var i = 0; i < 12 && Boolean(walker.parent); ++i) {
            walker = walker.parent
            if (walker.contentY !== undefined && walker.contentHeight !== undefined) {
                var pos = highlighter.found.mapToItem(walker.contentItem, 0, 0)
                walker.contentY = Math.max(0, Math.min(pos.y - 40, walker.contentHeight - walker.height))
                break
            }
        }

        highlighter.found.forceActiveFocus()
        holdTimer.restart()
    }

    Timer {
        id: holdTimer

        interval: highlighter.holdDuration
        repeat: false

        onTriggered: {
            highlighter.found = null
            highlighter.target = ""
            highlighter.cleared()
        }
    }

    Rectangle {
        id: pulse

        visible: highlighter.found !== null
        color: "transparent"
        radius: M3.shape.medium
        border.width: M3.focusIndicatorThickness
        border.color: M3.color.primary

        x: highlighter.found !== null ? highlighter.found.mapToItem(highlighter, 0, 0).x - 4 : 0
        y: highlighter.found !== null ? highlighter.found.mapToItem(highlighter, 0, 0).y - 4 : 0
        width: highlighter.found !== null ? highlighter.found.width + 8 : 0
        height: highlighter.found !== null ? highlighter.found.height + 8 : 0

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: M3.color.primary
            opacity: 0.12
        }

        // The pulse itself. Under reduced motion every duration token is zero,
        // so the animation settles immediately at full opacity and the
        // highlight is simply shown and then removed.
        SequentialAnimation on opacity {
            running: highlighter.found !== null
            loops: M3.motion.reducedMotion ? 1 : 3

            NumberAnimation {
                from: 1.0
                to: 0.35
                duration: M3.motion.long2
                easing: M3.motion.standard
            }

            NumberAnimation {
                from: 0.35
                to: 1.0
                duration: M3.motion.long2
                easing: M3.motion.standard
            }
        }
    }
}
