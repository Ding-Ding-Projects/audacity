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

    title: qsTrc("appshell/about", "About Audacity")

    contentHeight: 600
    contentWidth: 720

    AboutModel {
        id: aboutModel
    }

    NavigationPanel {
        id: tabsNavPanel
        name: "AboutDialogTabs"
        section: root.navigationSection
        order: 1
        direction: NavigationPanel.Horizontal
        accessible.name: qsTrc("appshell/about", "About Audacity")
    }

    NavigationPanel {
        id: buttonsNavPanel
        name: "AboutDialogButtons"
        section: root.navigationSection
        order: 2
        direction: NavigationPanel.Horizontal
    }

    Rectangle {
        anchors.fill: parent
        color: M3.color.surface
    }

    QtObject {
        id: prv

        readonly property int tabSpacing: 16
        readonly property int tabButtonSpacing: 32
        readonly property int btnMargins: 8
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: prv.tabSpacing

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 48

            M3Tabs {
                id: tabBar

                width: parent.width
                height: parent.height

                model: [
                    {
                        "text": qsTrc("appshell/about", "Audacity")
                    },
                    {
                        "text": qsTrc("appshell/about", "Legal")
                    }
                ]

                navigationPanel: tabsNavPanel
            }
        }

        StackLayout {
            id: stackLayout

            Layout.fillWidth: true
            Layout.fillHeight: true

            currentIndex: tabBar.currentIndex

            AboutDialogAudacityTab {
                model: aboutModel
            }

            AboutDialogPrivacyTab {
                model: aboutModel
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            M3Divider {
                Layout.fillWidth: true
            }

            M3Button {
                Layout.alignment: Qt.AlignRight
                Layout.margins: prv.btnMargins

                variant: "filled"

                //: Label of a dialog button
                text: qsTrc("global", "Close")

                navigation.panel: buttonsNavPanel

                onClicked: {
                    root.hide()
                }
            }
        }
    }
}
