/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.UiComponents

import Audacity.Experience
import Audacity.M3

PreferencesPage {
    id: root

    height: mainColumn.height

    ExperienceSettingsModel {
        id: settingsModel

        Component.onCompleted: settingsModel.load()
    }

    Column {
        id: mainColumn

        width: parent.width
        spacing: 16

        ExperienceLanguageSection {
            settingsModel: settingsModel

            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart + 1
        }

        M3Divider {}

        ExperienceToneSection {
            settingsModel: settingsModel

            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart + 2
        }

        M3Divider {}

        ExperienceAttentionSection {
            settingsModel: settingsModel

            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart + 3
        }

        M3Divider {}

        ExperienceScheduleSection {
            settingsModel: settingsModel

            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart + 4
        }

        M3Divider {}

        ExperienceVocabularySection {
            settingsModel: settingsModel

            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart + 5
        }
    }
}
