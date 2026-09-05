/*
 * Audacity: A Digital Audio Editor
 */
import QtQuick 2.15
import QtQuick.Layouts 1.15

import Muse.Ui 1.0
import Muse.UiComponents
import Audacity.M3
import Audacity.AppShell

StyledDialogView {
    id: root

    property alias isCreateAccountMode: signinPage.isCreateAccountMode

    title: isCreateAccountMode ? qsTrc("cloud", "Create an account on audio.com") : qsTrc("cloud", "Sign in to audio.com")

    contentHeight: content.implicitHeight
    contentWidth: content.implicitWidth

    margins: 20

    modal: true

    Rectangle {
        anchors.fill: parent
        color: M3.color.surface
    }

    ColumnLayout {
        id: content

        spacing: 0

        SigninAudiocomPage {
            id: signinPage

            Layout.preferredWidth: 560
            Layout.preferredHeight: 404

            navigationSection: root.navigationSection

            onNavNextPageRequested: {
                root.accept();
            }
        }

        NavigationPanel {
            id: buttonsNavPanel
            name: "SigninAudiocomButtons"
            section: root.navigationSection
            order: 1
            direction: NavigationPanel.Horizontal
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 12

            Item {
                Layout.fillWidth: true
            }

            M3Button {
                variant: "text"

                //: Label of a dialog button
                text: qsTrc("global", "Cancel")

                navigation.panel: buttonsNavPanel
                navigation.column: 0

                onClicked: {
                    root.reject();
                }
            }
        }
    }
}
