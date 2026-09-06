/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick

import Muse.UiComponents

import Audacity.Experience
import Audacity.M3

PreferencesPage {
    id: root

    // No explicit height override here: PreferencesPage's own default of
    // "height: parent.height" is what lets its internal Flickable actually
    // clip to the dialog and scroll. Overriding it to the content's own
    // height (as this page used to) makes the Flickable's height equal its
    // contentHeight, so nothing above the visible fold can ever be reached
    // by scrolling, keyboard focus-into-view, or a command palette
    // teleport landing on one of this page's later sections.

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

        M3Divider {}

        ExperienceDimSumSection {
            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart + 6
        }

        M3Divider {}

        ExperienceSchoolModeSection {
            settingsModel: settingsModel

            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart + 7
        }

        M3Divider {}

        ExperienceNarratorSection {
            settingsModel: settingsModel

            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart + 8
        }
    }
}
