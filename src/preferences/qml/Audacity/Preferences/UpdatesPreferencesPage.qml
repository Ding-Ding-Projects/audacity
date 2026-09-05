/*
* Audacity: A Digital Audio Editor
*/
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.SquirrelUpdate

PreferencesPage {
    id: root

    height: mainColumn.height

    SquirrelUpdateModel {
        id: updateModel

        Component.onCompleted: updateModel.load()
    }

    NavigationPanel {
        id: pageNavigation
        name: "SquirrelUpdatePreferencesPage"
        section: root.navigationSection
        order: root.navigationOrderStart + 1
        direction: NavigationPanel.Vertical
    }

    // State numbers mirror UpdateBannerState in squirrelupdatemodel.h.
    readonly property int stateNotApplicable: 12
    readonly property int stateChecking: 2
    readonly property int stateReady: 5
    readonly property int stateFailed: 6

    Column {
        id: mainColumn

        width: parent.width
        spacing: 16

        StyledTextLabel {
            width: parent.width
            font: M3.type.titleMedium
            color: M3.color.onSurface
            text: qsTrc("preferences", "Automatic updates")
        }

        StyledTextLabel {
            width: parent.width
            wrapMode: Text.WordWrap
            font: M3.type.bodyMedium
            color: M3.color.onSurfaceVariant
            visible: updateModel.state === root.stateNotApplicable
            text: qsTrc("preferences",
                        "Automatic updates are not applicable on this platform. Material Audacity is packaged with Squirrel.Windows, a Windows only installer, so this build must be replaced by hand.")
        }

        Column {
            width: parent.width
            spacing: 16
            visible: updateModel.state !== root.stateNotApplicable

            M3Switch {
                width: parent.width
                text: qsTrc("preferences", "Check for updates automatically")
                checked: updateModel.enabled
                onToggled: updateModel.enabled = checked

                navigation.name: "SquirrelUpdateEnabledSwitch"
                navigation.panel: pageNavigation
                navigation.row: 0
            }

            StyledTextLabel {
                width: parent.width
                wrapMode: Text.WordWrap
                font: M3.type.bodySmall
                color: M3.color.onSurfaceVariant
                text: qsTrc("preferences",
                            "This build is unsigned. A check downloads a small release feed over HTTPS and compares its hash, and never claims to verify a signature.")
            }

            StyledTextLabel {
                width: parent.width
                text: qsTrc("preferences", "Feed URL: %1").arg(updateModel.feedUrl)
                font: M3.type.bodySmall
                color: M3.color.onSurfaceVariant
                wrapMode: Text.WrapAnywhere
            }

            StyledTextLabel {
                width: parent.width
                text: qsTrc("preferences", "Check interval: every %1 hours")
                      .arg(updateModel.checkIntervalHours)
                font: M3.type.bodySmall
                color: M3.color.onSurfaceVariant
            }

            StyledTextLabel {
                width: parent.width
                text: qsTrc("preferences", "Last check: %1").arg(updateModel.lastCheckDisplay)
                font: M3.type.bodySmall
                color: M3.color.onSurfaceVariant
            }

            StyledTextLabel {
                width: parent.width
                wrapMode: Text.WordWrap
                visible: updateModel.state === root.stateFailed
                text: qsTrc("preferences", "The last check failed: %1").arg(updateModel.lastError)
                font: M3.type.bodySmall
                color: M3.color.error
            }

            StyledTextLabel {
                width: parent.width
                wrapMode: Text.WordWrap
                visible: updateModel.state === root.stateReady
                text: qsTrc("preferences", "Version %1 is downloaded, verified and ready to install.")
                      .arg(updateModel.availableVersion)
                font: M3.type.bodySmall
                color: M3.color.primary
            }

            M3Button {
                text: updateModel.state === root.stateChecking
                      ? qsTrc("preferences", "Checking…")
                      : qsTrc("preferences", "Check for updates")
                variant: "outlined"
                enabled: updateModel.state !== root.stateChecking
                onClicked: updateModel.checkForUpdate()

                navigation.name: "SquirrelUpdateCheckButton"
                navigation.panel: pageNavigation
                navigation.row: 1
            }
        }
    }
}
