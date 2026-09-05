/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick 2.15

import Muse.Ui 1.0
import Muse.UiComponents

import Audacity.UiComponents 1.0
import Audacity.M3

BaseSection {
    id: root

    title: qsTrc("preferences", "Export behavior")

    navigation.name: "ExportBehaviorSection"

    navigationOrderEnd: root.navigation.order

    required property var exportPreferencesModel

    M3Switch {
        id: m3Switch4
        width: parent.width

        text: qsTrc("preferences", "Show ‘How would you like to export?’ dialog")

        checked: root.exportPreferencesModel.askExportLocationType

        navigation.name: "AskExportLocationTypeCheckBox"
        navigation.panel: root.navigation
        navigation.row: 0

        onToggled: {
            root.exportPreferencesModel.askExportLocationType = checked;
            m3Switch4.checked = Qt.binding(function () {
                return root.exportPreferencesModel.askExportLocationType;
            });
        }
    }
}
