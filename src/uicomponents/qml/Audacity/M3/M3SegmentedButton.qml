/*
* Audacity: A Digital Audio Editor
*
* M3SegmentedButton
*
* A row of connected segments where either one or several may be selected. The
* outer corners are fully rounded and the inner edges share a single outline.
*
* Replaces: Muse.UiComponents RadioButtonGroup used as a toolbar selector.
*
* API:
*     model (list of { text, icon } or plain strings), currentIndex,
*     multiSelect, checkedIndexes, activated(index), navigationPanel
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Item {
    id: root

    property var model: []
    property int currentIndex: 0

    // When true several segments can be checked at once.
    property bool multiSelect: false
    property var checkedIndexes: []

    property NavigationPanel navigationPanel: null
    property int navigationRowStart: 0

    signal activated(int index)

    implicitHeight: M3.density.apply(40)
    implicitWidth: row.implicitWidth

    function isChecked(index) {
        if (root.multiSelect) {
            return root.checkedIndexes.indexOf(index) !== -1
        }
        return root.currentIndex === index
    }

    function toggle(index) {
        if (root.multiSelect) {
            var next = root.checkedIndexes.slice()
            var at = next.indexOf(index)
            if (at === -1) {
                next.push(index)
            } else {
                next.splice(at, 1)
            }
            root.checkedIndexes = next
        } else {
            root.currentIndex = index
        }
        root.activated(index)
    }

    Row {
        id: row

        anchors.fill: parent
        spacing: 0

        Repeater {
            id: repeater

            model: root.model

            delegate: Item {
                id: segment

                required property int index
                required property var modelData

                readonly property bool isFirst: segment.index === 0
                readonly property bool isLast: segment.index === repeater.count - 1
                readonly property bool checked: root.isChecked(segment.index)

                readonly property string label: typeof segment.modelData === "string"
                                                ? segment.modelData
                                                : (segment.modelData.text !== undefined ? segment.modelData.text : "")
                readonly property int iconCode: typeof segment.modelData === "object"
                                                && segment.modelData.icon !== undefined
                                                ? segment.modelData.icon : IconCode.NONE

                height: root.height
                width: Math.max(48, segmentContent.implicitWidth + 24)

                NavigationControl {
                    id: segmentNav

                    name: "M3Segment" + segment.index
                    panel: root.navigationPanel
                    row: root.navigationRowStart
                    column: segment.index
                    enabled: root.enabled && root.visible
                    accessible.role: MUAccessible.CheckBox
                    accessible.name: segment.label
                    accessible.checked: segment.checked

                    onTriggered: root.toggle(segment.index)
                }

                Rectangle {
                    id: segmentBackground

                    anchors.fill: parent
                    color: segment.checked ? M3.color.secondaryContainer : "transparent"
                    border.width: 1
                    border.color: M3.color.outline
                    antialiasing: true

                    topLeftRadius: segment.isFirst ? root.height / 2 : 0
                    bottomLeftRadius: segment.isFirst ? root.height / 2 : 0
                    topRightRadius: segment.isLast ? root.height / 2 : 0
                    bottomRightRadius: segment.isLast ? root.height / 2 : 0

                    // Share the outline with the neighbour on the left.
                    anchors.leftMargin: segment.isFirst ? 0 : -1

                    Behavior on color {
                        ColorAnimation {
                            duration: M3.motion.short3
                            easing: M3.motion.standard
                        }
                    }

                    M3StateLayer {
                        anchors.fill: parent
                        color: segment.checked ? M3.color.onSecondaryContainer : M3.color.onSurface
                        active: root.enabled
                        hovered: segmentMouse.containsMouse
                        pressed: segmentMouse.containsPress
                        focused: segmentNav.highlight
                    }
                }

                M3FocusRing {
                    anchors.fill: segmentBackground
                    shapeRadius: root.height / 2
                    visible: segmentNav.highlight
                }

                Row {
                    id: segmentContent

                    anchors.centerIn: parent
                    spacing: 8

                    StyledIconLabel {
                        anchors.verticalCenter: parent.verticalCenter
                        iconCode: segment.checked ? IconCode.TICK_RIGHT_ANGLE : segment.iconCode
                        visible: segment.checked || segment.iconCode !== IconCode.NONE
                        color: segment.checked ? M3.color.onSecondaryContainer : M3.color.onSurface
                    }

                    StyledTextLabel {
                        anchors.verticalCenter: parent.verticalCenter
                        text: segment.label
                        visible: segment.label !== ""
                        font: M3.typography.labelLarge
                        color: segment.checked ? M3.color.onSecondaryContainer : M3.color.onSurface
                    }
                }

                MouseArea {
                    id: segmentMouse

                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: root.enabled
                    cursorShape: Qt.PointingHandCursor

                    onClicked: {
                        segmentNav.requestActive()
                        root.toggle(segment.index)
                    }
                }
            }
        }
    }
}
