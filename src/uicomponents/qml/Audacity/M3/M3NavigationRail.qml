/*
* Audacity: A Digital Audio Editor
*
* M3NavigationRail
*
* The Material 3 navigation rail: a narrow vertical strip of destinations with
* an optional floating action button at the top. The active destination shows a
* pill shaped active indicator behind its icon.
*
* API:
*     model (list of { text, icon, badgeCount }), currentIndex, showLabels,
*     fabIcon, activated(index), fabTriggered(), navigationPanel
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
    property bool showLabels: true

    property int fabIcon: IconCode.NONE

    property NavigationPanel navigationPanel: null

    signal activated(int index)
    signal fabTriggered()

    implicitWidth: 80
    implicitHeight: 400

    Rectangle {
        anchors.fill: parent
        color: M3.color.surface
    }

    M3FAB {
        id: fab

        visible: root.fabIcon !== IconCode.NONE
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 44
        size: "small"
        variant: "primary"
        lowered: true
        icon: root.fabIcon
        accessibleName: "Primary action"

        onClicked: root.fabTriggered()
    }

    Column {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: fab.visible ? fab.bottom : parent.top
        anchors.topMargin: fab.visible ? 40 : 44
        spacing: 12

        Repeater {
            id: repeater

            model: root.model

            delegate: FocusScope {
                id: destination

                required property int index
                required property var modelData

                readonly property bool selected: root.currentIndex === destination.index
                readonly property string label: typeof destination.modelData === "string"
                                                ? destination.modelData
                                                : (destination.modelData.text !== undefined
                                                   ? destination.modelData.text : "")
                readonly property int iconCode: typeof destination.modelData === "object"
                                                && destination.modelData.icon !== undefined
                                                ? destination.modelData.icon : IconCode.NONE

                width: 56
                height: root.showLabels ? 56 : 32

                NavigationControl {
                    id: destinationNav

                    name: "M3RailDestination" + destination.index
                    panel: root.navigationPanel
                    row: destination.index
                    enabled: root.enabled && root.visible
                    accessible.role: MUAccessible.ListItem
                    accessible.name: destination.label
                    accessible.selected: destination.selected
                    accessible.visualItem: indicator

                    onTriggered: {
                        root.currentIndex = destination.index
                        root.activated(destination.index)
                    }
                }

                Rectangle {
                    id: indicator

                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    width: 56
                    height: 32
                    radius: 16
                    antialiasing: true
                    color: destination.selected ? M3.color.secondaryContainer : "transparent"

                    Behavior on color {
                        ColorAnimation {
                            duration: M3.motion.short4
                            easing: M3.motion.emphasized
                        }
                    }

                    M3StateLayer {
                        anchors.fill: parent
                        radius: indicator.radius
                        color: destination.selected ? M3.color.onSecondaryContainer : M3.color.onSurface
                        active: root.enabled
                        hovered: destinationMouse.containsMouse
                        pressed: destinationMouse.containsPress
                        focused: destinationNav.highlight
                    }

                    StyledIconLabel {
                        anchors.centerIn: parent
                        iconCode: destination.iconCode
                        color: destination.selected ? M3.color.onSecondaryContainer
                                                    : M3.color.onSurfaceVariant
                    }
                }

                M3FocusRing {
                    anchors.fill: indicator
                    shapeRadius: indicator.radius
                    visible: destinationNav.highlight
                }

                StyledTextLabel {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: indicator.bottom
                    anchors.topMargin: 4
                    visible: root.showLabels
                    text: destination.label
                    font: M3.typography.labelMedium
                    color: destination.selected ? M3.color.onSurface : M3.color.onSurfaceVariant
                }

                MouseArea {
                    id: destinationMouse

                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: root.enabled
                    cursorShape: Qt.PointingHandCursor

                    onClicked: {
                        destinationNav.requestActive()
                        root.currentIndex = destination.index
                        root.activated(destination.index)
                    }
                }
            }
        }
    }
}
