/*
* Audacity: A Digital Audio Editor
*/
import QtQuick 2.15
import QtQuick.Layouts 1.15

import Muse.Ui 1.0
import Muse.UiComponents

import Audacity.M3

Item {
    id: root

    property alias canRemove: deleteButton.enabled

    property NavigationPanel navigationPanel: NavigationPanel {
        name: "LabelEditorTopPanel"
        direction: NavigationPanel.Horizontal
        accessible.role: MUAccessible.Button
        accessible.name: titleLabel.text + "; " + importButton.text
    }

    implicitHeight: 48

    signal importRequested
    signal exportRequested
    signal removeSelectedLabelsRequested
    signal addLabelRequested

    function focusOnFirst() {
        importButton.navigation.requestActive()
    }

    function readInfo() {
        accessibleInfo.ignored = false
        accessibleInfo.focused = true
    }

    AccessibleItem {
        id: accessibleInfo
        visualItem: root
        role: MUAccessible.Button
        name: titleLabel.text
    }

    RowLayout {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.right: parent.right
        anchors.rightMargin: 12

        spacing: 8

        StyledTextLabel {
            id: titleLabel
            Layout.alignment: Qt.AlignLeft

            text: qsTrc("projectscene", "Labels")
            font: M3.typography.titleMedium
        }

        Item {
            Layout.fillWidth: true
        }

        M3Button {
            id: importButton

            Layout.alignment: Qt.AlignRight

            //: Label of the button that imports labels from a file
            text: qsTrc("projectscene", "Import")
            minWidth: 0
            horizontalPadding: 12

            navigation.name: "ImportButton"
            navigation.panel: root.navigationPanel
            navigation.order: 1

            onClicked: {
                root.importRequested()
            }
        }

        M3Button {
            id: exportButton

            Layout.alignment: Qt.AlignRight

            //: Label of the button that exports labels to a file
            text: qsTrc("projectscene", "Export")
            minWidth: 0
            horizontalPadding: 12

            navigation.name: "ExportButton"
            navigation.panel: root.navigationPanel
            navigation.order: importButton.navigation.order + 1

            onClicked: {
                root.exportRequested()
            }
        }

        SeparatorLine {}

        M3Button {
            id: deleteButton

            Layout.alignment: Qt.AlignRight

            //: Label of the button that deletes the selected labels
            text: qsTrc("projectscene", "Delete")
            minWidth: 0
            horizontalPadding: 12

            navigation.name: "DeleteButton"
            navigation.panel: root.navigationPanel
            navigation.order: exportButton.navigation.order + 1

            onClicked: {
                root.removeSelectedLabelsRequested()
            }
        }

        M3Button {
            id: addButton

            Layout.alignment: Qt.AlignRight

            text: qsTrc("projectscene", "Add label")
            minWidth: 0
            horizontalPadding: 12

            navigation.name: "AddButton"
            navigation.panel: root.navigationPanel
            navigation.order: deleteButton.navigation.order + 1

            onClicked: {
                root.addLabelRequested()
            }
        }
    }
}
