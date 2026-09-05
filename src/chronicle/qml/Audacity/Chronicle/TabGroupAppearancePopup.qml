/*
* Audacity: A Digital Audio Editor
*
* TabGroupAppearancePopup
*
* The editor behind "Edit group appearance…". It pairs the Material 3 colour
* picker with a name field, so a group carries both a colour and a name and is
* never identified by colour alone.
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Chronicle

StyledPopupView {
    id: root

    property TabStripModel tabModel: null
    property string groupId: ""

    cornerRadius: M3.shape.large
    elevationLevel: 3
    backgroundColor: M3.surfaceAt(3)
    borderColor: M3.color.outlineVariant

    contentWidth: 372
    contentHeight: 660

    property string groupName: ""
    property string groupColor: "#926BFF"

    readonly property var group: {
        if (!root.tabModel || root.groupId === "") {
            return null
        }
        var groups = root.tabModel.groups
        for (var i = 0; i < groups.length; ++i) {
            if (groups[i].id === root.groupId) {
                return groups[i]
            }
        }
        return null
    }

    onGroupChanged: {
        if (root.group) {
            root.groupName = root.group.name
            root.groupColor = root.group.color
        }
    }

    NavigationPanel {
        id: navPanel

        name: "TabGroupAppearance"
        enabled: root.isOpened
        direction: NavigationPanel.Vertical
    }

    Column {
        width: root.contentWidth
        spacing: 12

        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignLeft
            text: qsTrc("chronicle", "Group appearance")
            font: M3.typography.titleMedium
            color: M3.color.onSurface
        }

        M3TextField {
            id: nameField

            width: parent.width
            label: qsTrc("chronicle", "Group name")
            currentText: root.groupName
            navigation.panel: navPanel

            onTextEdited: function (text) {
                root.groupName = text
            }
        }

        M3ColorPicker {
            id: picker

            width: parent.width
            selection: root.groupColor
            navigationPanel: navPanel

            onSelectionChanged: root.groupColor = picker.selection
        }

        Row {
            spacing: 8

            M3Button {
                text: qsTrc("chronicle", "Cancel")
                variant: "text"
                navigation.panel: navPanel
                onClicked: root.close()
            }

            M3Button {
                text: qsTrc("chronicle", "Apply")
                variant: "filled"
                navigation.panel: navPanel

                onClicked: {
                    if (root.tabModel) {
                        root.tabModel.setGroupAppearance(root.groupId, root.groupName, root.groupColor)
                    }
                    root.close()
                }
            }
        }
    }
}
