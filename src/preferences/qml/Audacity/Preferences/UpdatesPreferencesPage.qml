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

    readonly property bool notApplicable: updateModel.state === root.stateNotApplicable

    Column {
        id: mainColumn

        width: parent.width
        spacing: 16

        StyledTextLabel {
            width: parent.width
            font: M3.typography.titleMedium
            color: M3.color.onSurface
            text: qsTrc("preferences", "Automatic updates")
        }

        // Every preferences page is created up front by the dialog, well
        // before the user ever navigates to it, and only becomes visible
        // later when it becomes the current page in the dialog's page
        // stack. An item whose own "visible" property, or an ancestor's
        // height, starts out bound to a false/zero value and is only later
        // flipped by a state change does not reliably repaint on this
        // build: it stays permanently blank even once every other property
        // (size, colour, the flipped boolean itself) reports the correct,
        // expected value. Every label below instead stays visible, laid
        // out and full height always; only its own text switches, to the
        // empty string when this platform can update. Changing a Text
        // item's text is an ordinary, safe operation that this build
        // handles correctly regardless of that history, and empty text
        // naturally collapses a label's own implicit height to (close to)
        // zero without ever touching "visible" or wrapping it in a
        // container whose height is toggled. The switch and the button
        // cannot be text-blanked the same way, so those two keep an
        // ordinary "visible" binding.
        StyledTextLabel {
            id: notApplicableLabel
            width: parent.width
            wrapMode: Text.WordWrap
            font: M3.typography.bodyMedium
            color: M3.color.onSurfaceVariant
            text: root.notApplicable ? qsTrc("preferences", "Automatic updates are not applicable on this platform. Material Audacity is packaged with Squirrel.Windows, a Windows only installer, so this build must be replaced by hand.") : ""
        }

        M3Switch {
            width: parent.width
            visible: !root.notApplicable
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
            font: M3.typography.bodySmall
            color: M3.color.onSurfaceVariant
            text: root.notApplicable ? "" : qsTrc("preferences", "This build is unsigned. A check downloads a small release feed over HTTPS and compares its hash, and never claims to verify a signature.")
        }

        StyledTextLabel {
            width: parent.width
            wrapMode: Text.WrapAnywhere
            font: M3.typography.bodySmall
            color: M3.color.onSurfaceVariant
            text: root.notApplicable ? "" : qsTrc("preferences", "Feed URL: %1").arg(updateModel.feedUrl)
        }

        StyledTextLabel {
            width: parent.width
            font: M3.typography.bodySmall
            color: M3.color.onSurfaceVariant
            text: root.notApplicable ? "" : qsTrc("preferences", "Check interval: every %1 hours").arg(updateModel.checkIntervalHours)
        }

        StyledTextLabel {
            width: parent.width
            font: M3.typography.bodySmall
            color: M3.color.onSurfaceVariant
            text: root.notApplicable ? "" : qsTrc("preferences", "Last check: %1").arg(updateModel.lastCheckDisplay)
        }

        StyledTextLabel {
            width: parent.width
            wrapMode: Text.WordWrap
            font: M3.typography.bodySmall
            color: M3.color.error
            text: (!root.notApplicable && updateModel.state === root.stateFailed) ? qsTrc("preferences", "The last check failed: %1").arg(updateModel.lastError) : ""
        }

        StyledTextLabel {
            width: parent.width
            wrapMode: Text.WordWrap
            font: M3.typography.bodySmall
            color: M3.color.primary
            text: (!root.notApplicable && updateModel.state === root.stateReady) ? qsTrc("preferences", "Version %1 is downloaded, verified and ready to install.").arg(updateModel.availableVersion) : ""
        }

        M3Button {
            visible: !root.notApplicable
            text: updateModel.state === root.stateChecking ? qsTrc("preferences", "Checking…") : qsTrc("preferences", "Check for updates")
            variant: "outlined"
            enabled: updateModel.state !== root.stateChecking
            onClicked: updateModel.checkForUpdate()

            navigation.name: "SquirrelUpdateCheckButton"
            navigation.panel: pageNavigation
            navigation.row: 1
        }
    }
}
