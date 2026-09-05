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

    EditPreferencesModel {
        id: editPreferencesModel
    }

    Component.onCompleted: {
        editPreferencesModel.init();
    }

    Column {

        width: parent.width
        spacing: root.sectionsSpacing

        EffectBehaviorSection {
            id: effectBehaviorSection

            editPreferencesModel: editPreferencesModel

            navigation.section: root.navigationSection
            navigation.order: root.navigationOrderStart

            onFocusChanged: {
                if (activeFocus) {
                    root.ensureContentVisibleRequested(Qt.rect(x, y, width, height));
                }
            }
        }

        M3Divider {}

        DeleteBehaviorSection {
            id: deleteBehaviorSection

            editPreferencesModel: editPreferencesModel
            parentBackgroundColor: root.color

            navigation.section: root.navigationSection
            navigation.order: effectBehaviorSection.navigationOrderEnd + 1

            onFocusChanged: {
                if (activeFocus) {
                    root.ensureContentVisibleRequested(Qt.rect(x, y, width, height));
                }
            }
        }

        M3Divider {}

        PasteBehaviorSection {
            id: pasteBehaviorSection

            editPreferencesModel: editPreferencesModel
            parentBackgroundColor: root.color

            navigation.section: root.navigationSection
            navigation.order: deleteBehaviorSection.navigationOrderEnd + 1

            onFocusChanged: {
                if (activeFocus) {
                    root.ensureContentVisibleRequested(Qt.rect(x, y, width, height));
                }
            }
        }

        M3Divider {}

        AsymmetricStereoHeightsSection {
            id: asymmetricStereoHeightsSection

            editPreferencesModel: editPreferencesModel

            navigation.section: root.navigationSection
            navigation.order: pasteBehaviorSection.navigationOrderEnd + 1

            onFocusChanged: {
                if (activeFocus) {
                    root.ensureContentVisibleRequested(Qt.rect(x, y, width, height));
                }
            }
        }

        M3Divider {}

        MonoStereoConversionSection {
            id: monoStereoConversionSection

            askBeforeConverting: editPreferencesModel.askBeforeConvertingToMonoOrStereo

            navigation.section: root.navigationSection
            navigation.order: asymmetricStereoHeightsSection.navigationOrderEnd + 1

            onFocusChanged: {
                if (activeFocus) {
                    root.ensureContentVisibleRequested(Qt.rect(x, y, width, height));
                }
            }

            onAskBeforeConvertingChanged: {
                editPreferencesModel.askBeforeConvertingToMonoOrStereo = askBeforeConverting;
            }
        }

        M3Divider {}

        ZoomToggleSection {
            id: zoomToggleSection

            zoomPresetModel: editPreferencesModel.zoomPresetList
            zoomPreset1: editPreferencesModel.zoomPreset1
            zoomPreset2: editPreferencesModel.zoomPreset2

            navigation.section: root.navigationSection
            navigation.order: monoStereoConversionSection.navigationOrderEnd + 1

            onZoomPreset1ChangeRequested: function (preset) {
                editPreferencesModel.setZoomPreset1(preset);
            }

            onZoomPreset2ChangeRequested: function (preset) {
                editPreferencesModel.setZoomPreset2(preset);
            }

            onFocusChanged: {
                if (activeFocus) {
                    root.ensureContentVisibleRequested(Qt.rect(x, y, width, height));
                }
            }
        }
    }
}
