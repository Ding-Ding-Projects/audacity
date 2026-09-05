/*
* Audacity: A Digital Audio Editor
*
* M3ToolTipHandler (internal)
*
* Shows and hides a muse tooltip for a hovered component. Keeping it in one
* place means every M3 component gets the same delay, placement and dismissal
* behaviour as the rest of the application.
*/
import QtQuick

Item {
    id: root

    // The item the tooltip is anchored to.
    property Item target: null

    property string title: ""
    property string description: ""
    property string shortcut: ""

    property bool hovered: false

    visible: false

    // The muse tooltip provider is a global context property, so these calls
    // cannot be qualified by an id.
    // qmllint disable unqualified
    onHoveredChanged: {
        if (!root.target || root.title === "") {
            return
        }
        if (root.hovered) {
            ui.tooltip.show(root.target, root.title, root.description, root.shortcut)
        } else {
            ui.tooltip.hide(root.target)
        }
    }

    Component.onDestruction: {
        if (root.target) {
            ui.tooltip.hide(root.target, true)
        }
    }
    // qmllint enable unqualified
}
