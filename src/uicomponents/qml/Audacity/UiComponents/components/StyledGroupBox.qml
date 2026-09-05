/*
* Audacity: A Digital Audio Editor
*/
import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15

import Muse.UiComponents
import Muse.Ui 1.0

import Audacity.M3

Column {
    id: root

    property string title: ""

    property int titleSpacing: 4
    property int margin: 16
    property int itemSpacing: 8

    property int borderWidth: 0

    property alias navPanel: meterStyleGroup.navigation

    property int value

    property bool enabled: true

    property color backgroundColor: M3.color.surfaceContainerHighest

    property var model: null

    spacing: root.titleSpacing

    signal valueChangeRequested(int value)

    StyledTextLabel {
        width: parent.width

        text: root.title
        font: M3.typography.titleSmall
        color: M3.color.onSurfaceVariant
        horizontalAlignment: Text.AlignLeft
    }

    Rectangle {
        width: parent.width
        height: meterStyleGroup.implicitHeight + 2 * root.margin

        color: root.backgroundColor
        border.width: root.borderWidth
        border.color: M3.color.outlineVariant
        radius: M3.shape.medium

        Item {
            anchors.fill: parent
            anchors.margins: root.margin

            RadioButtonGroup {
                id: meterStyleGroup

                orientation: Qt.Vertical
                width: parent.width

                spacing: root.itemSpacing

                model: root.model

                delegate: M3RadioButton {
                    id: meterStyleButton
                    text: modelData["label"]
                    checked: root.value == modelData["value"]
                    enabled: root.enabled

                    navigation.panel: root.navPanel
                    navigation.name: modelData["label"]
                    navigation.order: index

                    onToggled: {
                        root.valueChangeRequested(modelData["value"])
                    }
                }
            }
        }
    }
}
