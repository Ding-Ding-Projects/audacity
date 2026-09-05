/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick 2.15
import QtQuick.Layouts 1.15

import Muse.Ui 1.0
import Muse.UiComponents
import Audacity.M3
import Audacity.AppShell

DoublePage {
    id: root

    showRightContent: false
    title: workspaceModel.pageTitle

    navigationPanel.direction: NavigationPanel.Vertical
    navigationPanel.accessible.name: workspaceModel.navigationAccessibleName
    navigationPanel.accessible.description: workspaceModel.navigationAccessibleDescription

    // Page-level accessibility information
    AccessibleItem {
        id: pageAccessibleInfo

        accessibleParent: root.navigationSection.accessible
        visualItem: root
        role: MUAccessible.Panel

        name: root.title
        description: workspaceModel.pageAccessibleDescription
    }

    // Left side content
    leftContent: Column {
        anchors.fill: parent
        spacing: 16

        // Radio button options
        Column {
            spacing: 8
            width: parent.width

            Repeater {
                id: optionsRepeater

                model: workspaceModel.workspaces

                delegate: Rectangle {
                    border.color: modelData.selected ? M3.color.primary : M3.color.outlineVariant
                    border.width: modelData.selected ? 2 : 1
                    color: modelData.selected ? M3.color.secondaryContainer : "transparent"
                    height: 60
                    radius: M3.shape.medium
                    width: parent.width

                    Behavior on color {
                        ColorAnimation {
                            duration: M3.motion.short3
                            easing: M3.motion.standard
                        }
                    }

                    Row {
                        anchors.bottomMargin: 12
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.rightMargin: 16
                        anchors.topMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 12

                        M3RadioButton {
                            anchors.verticalCenter: parent.verticalCenter
                            checked: modelData.selected

                            navigation.name: "Workspace_" + modelData.code
                            navigation.panel: root.navigationPanel
                            navigation.column: 0
                            navigation.row: index
                            navigation.accessible.name: modelData.title
                            navigation.accessible.description: workspaceModel.formatNavigationDescription(modelData.description, modelData.selected)

                            onToggled: {
                                workspaceModel.selectWorkspace(modelData.code);
                            }
                        }
                        Column {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 4

                            Text {
                                font: M3.typography.titleSmall
                                color: M3.color.onSurface
                                text: modelData.title
                            }
                            Text {
                                anchors.left: parent.left
                                font: M3.typography.bodyMedium
                                color: M3.color.onSurfaceVariant
                                horizontalAlignment: Text.AlignLeft
                                text: modelData.description
                                wrapMode: Text.WordWrap
                            }
                        }
                    }
                    MouseArea {
                        anchors.fill: parent

                        onClicked: {
                            workspaceModel.selectWorkspace(modelData.code);
                        }
                    }

                    // Accessibility item for the entire workspace option
                    AccessibleItem {
                        accessibleParent: pageAccessibleInfo
                        visualItem: parent
                        role: MUAccessible.ListItem

                        name: modelData.title
                        description: workspaceModel.formatAccessibleDescription(modelData.description, modelData.selected)
                    }
                }
            }
        }

        // Additional info text
        Text {
            id: infoTextLabel
            font: M3.typography.bodyMedium
            color: M3.color.onSurfaceVariant
            horizontalAlignment: Text.AlignLeft
            text: workspaceModel.additionalInfoText
            width: parent.width
            wrapMode: Text.WordWrap

            // Accessibility for the info text
            AccessibleItem {
                accessibleParent: pageAccessibleInfo
                visualItem: infoTextLabel
                role: MUAccessible.StaticText

                name: workspaceModel.additionalInfoAccessibleName
                description: infoTextLabel.text
            }
        }
    }

    Component.onCompleted: {
        workspaceModel.load();
    }

    WorkspaceLayoutPageModel {
        id: workspaceModel
    }
}
