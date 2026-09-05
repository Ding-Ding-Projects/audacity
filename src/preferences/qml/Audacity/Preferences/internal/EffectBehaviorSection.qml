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

    title: qsTrc("preferences", "Effect behavior")

    property var editPreferencesModel: null

    navigationOrderEnd: root.navigation.order

    Column {
        width: parent.width
        spacing: 24

        M3Switch {
            id: checkbox

            width: parent.width

            text: qsTrc("preferences", "Apply effects to all audio when no selection is made")

            checked: editPreferencesModel.applyEffectToAllAudio

            navigation.name: "EmptySelectionEffectsBox"
            navigation.panel: root.navigation

            onToggled: {
                editPreferencesModel.setApplyEffectToAllAudio(checked)
                checkbox.checked = Qt.binding(function () {
                    return editPreferencesModel.applyEffectToAllAudio
                })
            }
        }
    }
}
