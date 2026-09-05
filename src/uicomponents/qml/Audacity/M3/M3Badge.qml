/*
* Audacity: A Digital Audio Editor
*
* M3Badge
*
* A small or large badge. The small badge is a six pixel dot. The large badge
* shows a number, capped at the configured maximum.
*
* API:
*     count, maxCount, showCount, accessibleText
*/
import QtQuick

import Muse.UiComponents

import Audacity.M3

Rectangle {
    id: root

    property int count: 0
    property int maxCount: 999

    // A large badge shows the count, a small badge is a plain dot.
    property bool showCount: true

    /*
     * A badge is decorative. The host component must fold accessibleText into
     * its own accessible name so that the count is announced once, in context.
     */
    readonly property string accessibleText: root.showCount ? root.displayText : ""

    readonly property string displayText: root.count > root.maxCount ? root.maxCount + "+" : String(root.count)

    visible: root.showCount ? root.count > 0 : true

    implicitHeight: root.showCount ? 16 : 6
    implicitWidth: root.showCount ? Math.max(16, countLabel.implicitWidth + 8) : 6

    radius: height / 2
    color: M3.color.error
    antialiasing: true

    StyledTextLabel {
        id: countLabel

        anchors.centerIn: parent
        visible: root.showCount
        text: root.displayText
        font: M3.typography.labelSmall
        color: M3.color.onError
    }
}
