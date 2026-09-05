/*
* Audacity: A Digital Audio Editor
*
* M3NavigationDrawer
*
* The Material 3 navigation drawer, standard or modal. Destinations are fully
* rounded rows with an active indicator, and the model may carry section
* headlines and dividers.
*
* API:
*     model (list of { text, icon, badgeCount, headline, separator }),
*     currentIndex, headline, modal, opened, activated(index),
*     open(), close(), navigationPanel
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
    property string headline: ""

    property bool modal: false
    property bool opened: true
    property real drawerWidth: 360

    property NavigationPanel navigationPanel: null

    signal activated(int index)
    signal closed()

    implicitWidth: root.modal ? 0 : root.drawerWidth
    implicitHeight: 600

    function open() {
        root.opened = true
    }

    function close() {
        root.opened = false
        root.closed()
    }

    Rectangle {
        anchors.fill: parent
        visible: root.modal
        color: M3.color.scrim
        opacity: root.opened ? 0.32 : 0.0

        Behavior on opacity {
            NumberAnimation {
                duration: M3.motion.medium2
                easing: M3.motion.emphasized
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: root.modal && root.opened
            onClicked: root.close()
        }
    }

    M3Surface {
        id: panel

        level: root.modal ? 1 : 0
        shadowVisible: root.modal

        width: root.drawerWidth
        height: root.height
        x: root.opened ? 0 : -root.drawerWidth

        topRightRadius: M3.shape.extraLarge
        bottomRightRadius: M3.shape.extraLarge

        Behavior on x {
            NumberAnimation {
                duration: M3.motion.medium4
                easing: M3.motion.emphasizedDecelerate
            }
        }

        StyledTextLabel {
            id: headlineLabel

            visible: root.headline !== ""
            anchors.left: parent.left
            anchors.leftMargin: 28
            anchors.right: parent.right
            anchors.rightMargin: 28
            anchors.top: parent.top
            anchors.topMargin: 16
            height: visible ? 56 : 0
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            text: root.headline
            font: M3.typography.titleSmall
            color: M3.color.onSurfaceVariant
        }

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: headlineLabel.bottom
            anchors.margins: 12
            spacing: 0

            Repeater {
                id: repeater

                model: root.model

                delegate: Item {
                    id: entry

                    required property int index
                    required property var modelData

                    readonly property bool isSeparator: entry.modelData.separator === true
                    readonly property bool isHeadline: !entry.isSeparator
                                                       && entry.modelData.headline !== undefined
                    readonly property bool isDestination: !entry.isSeparator && !entry.isHeadline
                    readonly property bool selected: root.currentIndex === entry.index

                    readonly property string label: {
                        if (entry.isHeadline) {
                            return String(entry.modelData.headline)
                        }
                        return entry.modelData.text !== undefined ? String(entry.modelData.text) : ""
                    }

                    width: parent.width
                    height: entry.isSeparator ? 17 : 56

                    M3Divider {
                        visible: entry.isSeparator
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.right: parent.right
                        inset: 16
                    }

                    StyledTextLabel {
                        visible: entry.isHeadline
                        anchors.fill: parent
                        leftPadding: 16
                        horizontalAlignment: Text.AlignLeft
                        verticalAlignment: Text.AlignVCenter
                        text: entry.label
                        font: M3.typography.titleSmall
                        color: M3.color.onSurfaceVariant
                    }

                    NavigationControl {
                        id: destinationNav

                        name: "M3DrawerDestination" + entry.index
                        panel: root.navigationPanel
                        row: entry.index
                        enabled: entry.isDestination && root.enabled && root.visible
                        accessible.role: MUAccessible.ListItem
                        accessible.name: entry.label
                        accessible.selected: entry.selected
                        accessible.visualItem: pill

                        onTriggered: {
                            root.currentIndex = entry.index
                            root.activated(entry.index)
                        }
                    }

                    Rectangle {
                        id: pill

                        visible: entry.isDestination
                        anchors.fill: parent
                        radius: height / 2
                        antialiasing: true
                        color: entry.selected ? M3.color.secondaryContainer : "transparent"

                        Behavior on color {
                            ColorAnimation {
                                duration: M3.motion.short4
                                easing: M3.motion.emphasized
                            }
                        }

                        M3StateLayer {
                            anchors.fill: parent
                            radius: pill.radius
                            color: entry.selected ? M3.color.onSecondaryContainer : M3.color.onSurface
                            active: root.enabled
                            hovered: destinationMouse.containsMouse
                            pressed: destinationMouse.containsPress
                            focused: destinationNav.highlight
                        }

                        StyledIconLabel {
                            id: destinationIcon

                            anchors.left: parent.left
                            anchors.leftMargin: 16
                            anchors.verticalCenter: parent.verticalCenter
                            iconCode: entry.modelData.icon !== undefined
                                      ? entry.modelData.icon : IconCode.NONE
                            color: entry.selected ? M3.color.onSecondaryContainer
                                                  : M3.color.onSurfaceVariant
                        }

                        StyledTextLabel {
                            anchors.left: destinationIcon.right
                            anchors.leftMargin: 12
                            anchors.right: badge.left
                            anchors.rightMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            horizontalAlignment: Text.AlignLeft
                            elide: Text.ElideRight
                            text: entry.label
                            font: M3.typography.labelLarge
                            color: entry.selected ? M3.color.onSecondaryContainer
                                                  : M3.color.onSurfaceVariant
                        }

                        M3Badge {
                            id: badge

                            anchors.right: parent.right
                            anchors.rightMargin: 24
                            anchors.verticalCenter: parent.verticalCenter
                            count: entry.modelData.badgeCount !== undefined
                                   ? entry.modelData.badgeCount : 0
                        }
                    }

                    M3FocusRing {
                        anchors.fill: pill
                        shapeRadius: pill.radius
                        visible: destinationNav.highlight
                    }

                    MouseArea {
                        id: destinationMouse

                        anchors.fill: parent
                        hoverEnabled: true
                        enabled: entry.isDestination && root.enabled
                        cursorShape: Qt.PointingHandCursor

                        onClicked: {
                            destinationNav.requestActive()
                            root.currentIndex = entry.index
                            root.activated(entry.index)
                        }
                    }
                }
            }
        }
    }
}
