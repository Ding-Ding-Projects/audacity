/*
* Audacity: A Digital Audio Editor
*
* PersonalizableItem
*
* Wrap any control or region with this item to give it "Edit appearance..."
* and "Lock this element..." on its right click menu, Shift+right-click as a
* direct route to the appearance editor, and the actual lock behaviour: while
* the element is actively locked, a click opens the unlock prompt instead of
* reaching whatever is underneath.
*
* API:
*     elementId (a stable identifier this element keeps across restarts),
*     elementLabel (for the lock wizard and the support desk),
*     extraMenuItems (additional entries placed above the two standard ones)
*/
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Personalize

Item {
    id: root

    default property alias content: contentContainer.data

    property string elementId: ""
    property string elementLabel: ""
    property var extraMenuItems: []

    signal extraMenuItemTriggered(string itemId)

    readonly property bool locked: elementId !== "" && LockRegistry.isActivelyLocked(elementId)

    Item {
        id: contentContainer
        anchors.fill: parent
        enabled: !root.locked
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton
        onClicked: function (mouse) {
            if (mouse.modifiers & Qt.ShiftModifier) {
                appearanceEditor.openAt(root, root.elementId)
            } else {
                var items = root.extraMenuItems.concat([
                    {
                        id: "editAppearance",
                        title: qsTrc("personalize", "Edit appearance…")
                    },
                    {
                        id: "lockElement",
                        title: root.locked ? qsTrc("personalize", "Lock again…") : (LockRegistry.isLocked(root.elementId) ? qsTrc("personalize", "Remove lock…") : qsTrc("personalize", "Lock this element…"))
                    }
                ])
                menu.model = items
                menu.toolTipTitle = root.elementLabel
                menu.open()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.locked
        color: M3.color.surfaceContainerHighest
        opacity: 0.55

        MouseArea {
            anchors.fill: parent
            onClicked: unlockPopover.openAt(root, root.elementId, root.elementLabel)
        }

        Column {
            anchors.centerIn: parent
            spacing: 4
            StyledIconLabel {
                iconCode: IconCode.LOCK_CLOSED
                anchors.horizontalCenter: parent.horizontalCenter
            }
            StyledTextLabel {
                text: qsTrc("personalize", "Locked, just for fun")
                font: M3.typography.labelSmall
            }
        }
    }

    M3Menu {
        id: menu

        onHandleMenuItem: function (itemId) {
            if (itemId === "editAppearance") {
                appearanceEditor.openAt(root, root.elementId)
            } else if (itemId === "lockElement") {
                if (LockRegistry.isLocked(root.elementId)) {
                    LockRegistry.removeLock(root.elementId)
                } else {
                    lockWizard.openAt(root, root.elementId, root.elementLabel)
                }
            } else {
                root.extraMenuItemTriggered(itemId)
            }
        }
    }

    AppearanceEditorPopover {
        id: appearanceEditor
    }

    LockWizardPopover {
        id: lockWizard
    }

    LockUnlockPopover {
        id: unlockPopover
    }
}
