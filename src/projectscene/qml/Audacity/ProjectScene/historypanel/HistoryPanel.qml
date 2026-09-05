/*
* Audacity: A Digital Audio Editor
*/
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import Muse.Ui
import Muse.UiComponents

import Audacity.ProjectScene

import Audacity.M3

Item {
    id: root

    property alias navigationSection: navPanel.section
    property alias navigationOrderStart: navPanel.order

    // The search field keeps the hook the regular expression builder connects
    // to, so the panel joins the application wide search contract.
    signal regexBuilderRequested

    NavigationPanel {
        id: navPanel
        name: "HistoryPanel"
        direction: NavigationPanel.Vertical
        enabled: root.enabled && root.visible
    }

    QtObject {
        id: prv

        property string filterText: ""

        // A search term is used as a regular expression when it is one, and as
        // plain text otherwise, so a typed bracket never empties the list.
        function matches(text) {
            if (prv.filterText === "") {
                return true
            }

            try {
                return new RegExp(prv.filterText, "i").test(text)
            } catch (error) {
                return String(text).toLowerCase().indexOf(prv.filterText.toLowerCase()) !== -1
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent

        spacing: 0

        M3SearchBar {
            id: searchBar

            Layout.fillWidth: true
            Layout.margins: 12

            placeholder: qsTrc("projectscene", "Search history")
            accessibleName: qsTrc("projectscene", "Search history")

            showRegexBuilder: true

            navigation.panel: navPanel
            navigation.order: -1

            onSearchTextChanged: {
                prv.filterText = searchBar.searchText
            }

            onRegexBuilderRequested: {
                root.regexBuilderRequested()
            }
        }

        M3Divider {
            Layout.fillWidth: true
        }

        StyledListView {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true

            model: HistoryPanelModel {
                id: historyPanelModel
            }

            currentIndex: historyPanelModel.currentIndex
            scrollBarPolicy: ScrollBar.AlwaysOn

            delegate: M3ListItem {
                id: listItem

                required property int index
                required property string text

                readonly property bool isRedoable: listItem.index > historyPanelModel.currentIndex
                readonly property bool isVisibleItem: prv.matches(listItem.text)

                width: listView.width
                height: listItem.isVisibleItem ? listItem.implicitHeight : 0
                visible: listItem.isVisibleItem

                headline: listItem.text
                accessibleName: listItem.text

                selected: listItem.index === historyPanelModel.currentIndex
                leadingIcon: listItem.selected ? IconCode.TICK_RIGHT_ANGLE : IconCode.NONE

                opacity: listItem.isRedoable ? 0.7 : 1

                navigation.panel: navPanel
                navigation.order: listItem.index
                navigation.accessible.row: listItem.index

                onClicked: {
                    historyPanelModel.undoRedoToIndex(listItem.index)
                }
            }
        }
    }
}
