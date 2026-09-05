/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.AppShell
import Audacity.Preferences
import Audacity.M3

PreferencesPage {
    id: root

    readonly property int navigationOrderEnd: saveBehaviorSection.navigationOrderEnd

    ExportPreferencesPageModel {
        id: exportPreferencesModel
    }

    Component.onCompleted: {
        exportPreferencesModel.init()
    }

    Column {
        width: parent.width
        spacing: root.sectionsSpacing

        ExportBehaviorSection {
            id: exportBehaviorSection

            exportPreferencesModel: exportPreferencesModel

            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart

            onFocusChanged: {
                if (activeFocus) {
                    root.ensureContentVisibleRequested(Qt.rect(x, y, width, height))
                }
            }
        }

        M3Divider {}

        SaveBehaviorSection {
            id: saveBehaviorSection

            exportPreferencesModel: exportPreferencesModel

            navigation.section: root.navigationSection
            navigation.order: exportBehaviorSection.navigationOrderEnd + 1

            onFocusChanged: {
                if (activeFocus) {
                    root.ensureContentVisibleRequested(Qt.rect(x, y, width, height))
                }
            }
        }
    }
}
