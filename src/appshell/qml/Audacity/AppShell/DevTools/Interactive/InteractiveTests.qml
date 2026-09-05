/*
* Audacity: A Digital Audio Editor
*/
import QtQuick 2.7
import Muse.Interactive
import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Rectangle {

    color: M3.color.surface

    NavigationSection {
        id: navSec
        name: "InteractiveTests"
        order: 10
    }

    NavigationPanel {
        id: navPanel
        name: "InteractiveTests"
        section: navSec
        order: 1
        direction: NavigationPanel.Vertical

        accessible.name: "InteractiveTests"
    }

    InteractiveTestsModel {
        id: testModel
    }

    Component.onCompleted: {
        testModel.init();
    }

    Text {
        id: header
        color: M3.color.onSurface
        width: parent.width
        height: 40
        text: testModel.currentUri
    }

    Grid {
        anchors.top: header.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 16
        spacing: 16
        columns: 2

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 0
            text: "[cpp] Sample dialog"
            onClicked: testModel.openSampleDialog()
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 1
            text: "[qml] Sample dialog"
            onClicked: {
                console.log("qml: before open");
                api.launcher.open("musescore://devtools/interactive/sample?color=#0F9D58&isApplyColor=true");
                console.log("qml: after open");
            }
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 2
            text: "[cpp] Sample dialog async"
            onClicked: testModel.openSampleDialogAsync()
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 3
            text: "[qml] Sample dialog sync"
            onClicked: {
                console.log("qml: before open");
                api.launcher.open("musescore://devtools/interactive/sample?sync=true&color=#EF8605");
                console.log("qml: after open");
            }
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 4
            text: "[qml] Sample dialog modal"
            onClicked: {
                console.log("qml: before open");
                api.launcher.open("musescore://devtools/interactive/sample?modal=true&color=#D13F31");
                console.log("qml: after open");
            }
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 5
            text: "[cpp] Sample dialog close"
            onClicked: testModel.closeSampleDialog()
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 6
            text: "Open musescore.com"
            onClicked: {
                api.launcher.openUrl("https://musescore.com/");
            }
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 7
            text: "Question"
            onClicked: testModel.question()
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 8
            text: "Custom question"
            onClicked: testModel.customQuestion()
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 9
            text: "Information"
            onClicked: testModel.information()
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 10
            text: "Warning"
            onClicked: testModel.warning()
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 11
            text: "Critical"
            onClicked: testModel.critical()
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 10
            text: "Critical with detailed text"
            onClicked: testModel.criticalWithDetailedText()
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 12
            text: "Require"
            onClicked: testModel.require()
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 13
            text: "Widget dialog"
            onClicked: testModel.openWidgetDialog()
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 14
            text: "Widget dialog async"
            onClicked: testModel.openWidgetDialogAsync()
        }

        M3Button {
            width: 200
            navigation.panel: navPanel
            navigation.row: 15
            text: "Widget dialog close"
            onClicked: testModel.closeWidgetDialog()
        }
    }
}
