/*
* Audacity: A Digital Audio Editor
*/
import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.SquirrelUpdate

// The persistent, non-blocking banner that offers an unsigned update once one
// has downloaded and verified. It never interrupts what the user is doing: it
// sits at a corner of the window and stays until dismissed or acted on.
//
// States: Hidden (0), NoUpdate (1), Checking (2), Available (3),
// Downloading (4), Ready (5), Failed (6), Offline (7), InvalidMetadata (8),
// CorruptAsset (9), Cancelled (10), Rollback (11), NotApplicable (12).
// Only Ready shows a banner; the rest are read from Preferences.
M3Card {
    id: root

    property SquirrelUpdateModel model: null

    signal restartRequested
    signal laterRequested

    visible: model !== null && model.bannerVisible
    variant: "elevated"
    padding: 16
    width: 400
    implicitHeight: layout.implicitHeight + root.padding * 2

    accessibleName: qsTrc("squirrelupdate", "Update ready. %1").arg(model ? model.availableVersion : "")

    Column {
        id: layout
        width: parent.width
        spacing: 8

        StyledTextLabel {
            width: parent.width
            font: M3.typography.titleSmall
            color: M3.color.onSurface
            wrapMode: Text.WordWrap
            text: qsTrc("squirrelupdate", "Material Audacity %1 is ready to install").arg(model ? model.availableVersion : "")
        }

        StyledTextLabel {
            width: parent.width
            font: M3.typography.bodySmall
            color: M3.color.onSurfaceVariant
            wrapMode: Text.WordWrap
            text: qsTrc("squirrelupdate", "This build is unsigned. No signature is checked, only the file's own hash from the release feed.")
        }

        Row {
            width: parent.width
            spacing: 8
            layoutDirection: Qt.RightToLeft

            M3Button {
                text: qsTrc("squirrelupdate", "Restart to install update")
                variant: "filled"
                navigation.name: "SquirrelUpdateRestart"
                navigation.panel: root.navigation ? root.navigation.panel : null
                onClicked: {
                    if (model) {
                        model.restartToUpdate()
                    }
                    root.restartRequested()
                }
            }

            M3Button {
                text: qsTrc("squirrelupdate", "Later")
                variant: "text"
                navigation.name: "SquirrelUpdateLater"
                navigation.panel: root.navigation ? root.navigation.panel : null
                onClicked: {
                    if (model) {
                        model.dismiss()
                    }
                    root.laterRequested()
                }
            }
        }
    }
}
