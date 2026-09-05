/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
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
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import Muse.Ui 1.0
import Muse.UiComponents
import Muse.Extensions 1.0

import Audacity.M3
import Audacity.Companion

FocusScope {
    id: root

    property alias color: background.color
    property string section: ""

    //! NOTE Hook for the regex builder, attached by the search lane.
    signal regexBuilderRequested

    QtObject {
        id: prv

        readonly property int sideMargin: 46
    }

    NavigationSection {
        id: navSec
        name: "Extensions"
        enabled: root.enabled && root.visible
        order: 3
        onActiveChanged: function (active) {
            if (active) {
                root.forceActiveFocus()
            }
        }
    }

    onSectionChanged: {
        if (!Boolean(root.section)) {
            return
        }

        tabBar.selectPage(root.section)
    }

    Rectangle {
        id: background
        anchors.fill: parent
        color: M3.color.surface
    }

    NavigationPanel {
        id: navTopPanel
        name: "PluginsTopPanel"
        section: navSec
        order: 1
        accessible.name: qsTrc("appshell", "Plugins")
    }

    Column {
        id: topLayout

        anchors.top: parent.top
        anchors.topMargin: prv.sideMargin
        anchors.left: parent.left
        anchors.leftMargin: prv.sideMargin
        anchors.right: parent.right
        anchors.rightMargin: prv.sideMargin

        spacing: 24

        RowLayout {
            width: parent.width

            spacing: 12

            Text {
                id: pageTitle
                Layout.fillWidth: true

                text: qsTrc("appshell", "Plugins")
                font: M3.typography.headlineSmall
                color: M3.color.onSurface
                horizontalAlignment: Text.AlignLeft
            }

            M3SearchBar {
                id: searchField

                objectName: "PluginsPageSearch"

                Layout.preferredWidth: 260

                placeholder: qsTrc("appshell", "Search plugins")
                accessibleName: qsTrc("appshell", "Plugins search")
                showRegexBuilder: true

                navigation.name: "PluginsSearch"
                navigation.panel: navTopPanel
                navigation.order: 1

                onSearchTextChanged: {
                    categoryDropdown.selectedCategory = ""
                }

                onRegexBuilderRequested: {
                    regexBuilder.pattern = searchField.searchText
                    regexBuilder.open()
                    root.regexBuilderRequested()
                }
            }

            M3Dropdown {
                id: categoryDropdown

                Layout.preferredWidth: searchField.width

                label: qsTrc("appshell", "Category")

                navigation.name: "CategoryDropdown"
                navigation.panel: navTopPanel
                navigation.order: 2

                readonly property string allCategoryValue: "ALL_CATEGORY"
                property string selectedCategory: (currentValue !== allCategoryValue) ? currentValue : ""

                currentIndex: 0

                function initModel() {
                    var categories = pluginsPage.categories()
                    var result = [];

                    //: The title of an option to display the plugins from all categories.
                    result.push({
                        "text": qsTrc("appshell", "All"),
                        "value": allCategoryValue
                    })

                    for (var i = 0; i < categories.length; ++i) {
                        var category = categories[i]
                        result.push({
                            "text": category.title,
                            "value": category.code
                        })
                    }

                    model = result
                }

                Component.onCompleted: {
                    initModel()
                }

                onActivated: function (index, value) {
                    currentIndex = index
                }
            }
        }

        M3Button {
            id: reloadButton

            anchors.right: parent.right

            text: qsTrc("extensions", "Reload plugins")
            icon: IconCode.UPDATE
            variant: "tonal"

            navigation.name: "PluginsReloadButton"
            navigation.panel: navTopPanel
            navigation.order: 3

            onClicked: {
                pluginsPage.reloadPlugins()
            }
        }
    }

    ExtensionsListPanel {
        id: pluginsPage

        anchors.top: topLayout.bottom
        anchors.topMargin: 24
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        search: searchField.searchText
        selectedCategory: categoryDropdown.selectedCategory
        backgroundColor: root.color

        sideMargin: prv.sideMargin

        navigationSection: navSec
    }

    // The regular expression builder for this field. Each search surface owns its
    // own instance, so its pattern, flags, sample and saved test cases are
    // isolated from every other search field in the application.
    RegexBuilderSheet {
        id: regexBuilder

        anchors.fill: parent

        storeName: "plugins-page"
        fieldLabel: "Plugins"

        onPatternAccepted: function (pattern) {
            searchField.searchText = pattern
        }
    }
}
