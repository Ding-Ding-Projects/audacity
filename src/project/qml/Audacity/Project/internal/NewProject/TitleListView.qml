/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Audacity-CLA-applies
 *
 * Audacity
 * Music Composition & Notation
 *
 * Copyright (C) 2024 Audacity BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
import QtQuick 2.15

import Muse.Ui 1.0
import Muse.UiComponents
import Audacity.M3
import Audacity.Companion

Item {
    id: root

    property alias listTitle: title.text
    property alias model: view.model

    property alias currentIndex: view.currentIndex

    property alias searchEnabled: searchField.visible
    property alias searchText: searchField.searchText
    readonly property bool searching: searchField.searchText !== ""

    property alias navigationPanel: view.navigation

    signal titleClicked(var index)

    signal doubleClicked(var index)

    function clearSearch() {
        searchField.clear()
    }

    StyledTextLabel {
        id: title

        anchors.top: parent.top

        font: M3.typography.titleSmall
    }

    M3SearchBar {
        id: searchField

        objectName: "NewProjectTitleSearch"

        showRegexBuilder: true

        onRegexBuilderRequested: {
            regexBuilder.pattern = searchField.searchText
            regexBuilder.open()
        }

        anchors.top: title.bottom
        anchors.topMargin: 16

        navigation.name: "Search"
        navigation.panel: view.navigation
        navigation.row: 1

        width: parent.width
    }

    StyledListView {
        id: view

        anchors.top: searchEnabled ? searchField.bottom : title.bottom
        anchors.topMargin: 16
        anchors.bottom: parent.bottom

        width: parent.width
        spacing: 0

        currentIndex: 0

        accessible.name: title.text

        delegate: ListItemBlank {
            id: item

            isSelected: view.currentIndex === model.index

            navigation.name: modelData
            navigation.panel: view.navigation
            navigation.row: 2 + model.index
            navigation.accessible.name: titleLabel.text
            navigation.accessible.row: model.index

            StyledTextLabel {
                id: titleLabel

                anchors.fill: parent
                anchors.leftMargin: 12

                horizontalAlignment: Text.AlignLeft
                text: modelData
                font: M3.typography.titleSmall
            }

            onClicked: {
                root.titleClicked(model.index)
            }

            onDoubleClicked: {
                root.doubleClicked(model.index)
            }
        }
    }

    StyledTextLabel {
        id: noResultsFoundHint

        anchors.fill: parent

        font: M3.typography.titleSmall

        //: Message shown when a search returns nothing
        text: qsTrc("global", "No results found")

        visible: view.count < 1
    }

    // The regular expression builder for this field. Each search surface owns its
    // own instance, so its pattern, flags, sample and saved test cases are
    // isolated from every other search field in the application.
    RegexBuilderSheet {
        id: regexBuilder

        anchors.fill: parent

        storeName: "new-project-titles"
        fieldLabel: "New project templates"

        onPatternAccepted: function (pattern) {
            searchField.searchText = pattern
        }
    }
}
