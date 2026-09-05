/*
* Audacity: A Digital Audio Editor
*
* Toolkit preferences page: entry points to the local model manager, the
* in-app documentation browser and external editor integration. Each opens
* full within a tab of the M3 tab strip rather than being crammed into one
* scrolling column.
*/
import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.Toolkit

PreferencesPage {
    id: root

    ColumnLayout {
        width: parent.width
        height: 560
        spacing: 8

        M3Tabs {
            id: tabs

            Layout.fillWidth: true

            model: [
                { "text": qsTrc("preferences", "Local model manager") },
                { "text": qsTrc("preferences", "Documentation") }
            ]
        }

        Loader {
            Layout.fillWidth: true
            Layout.fillHeight: true

            sourceComponent: tabs.currentIndex === 0 ? ollamaComponent : docsComponent
        }
    }

    Component {
        id: ollamaComponent

        OllamaPage {}
    }

    Component {
        id: docsComponent

        DocsBrowserPage {}
    }
}
