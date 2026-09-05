/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Audacity-CLA-applies
 *
 * Audacity
 * A Digital Audio Editor
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
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

M3Dialog {
    id: root

    title: qsTrc("preferences", "Preferences")
    headline: qsTrc("preferences", "Preferences")

    contentWidth: 960
    contentHeight: 660
    resizable: true

    fullScreen: false
    margins: 24

    property string currentPageId: ""
    property var params: null

    signal regexBuilderRequested

    QtObject {
        id: prv

        property var pagesObjects: (new Map())
        property var pages: []
        property var filteredPages: []
        property string searchText: ""

        function indexIn(pages, pageId) {
            for (var i = 0; i < pages.length; ++i) {
                if (pages[i].id === pageId) {
                    return i;
                }
            }
            return -1;
        }

        function updateStackCurrentIndex() {
            var stackIndex = prv.indexIn(prv.pages, preferencesModel.currentPageId);
            if (stackIndex >= 0) {
                stack.currentIndex = stackIndex;
            }

            var tabIndex = prv.indexIn(prv.filteredPages, preferencesModel.currentPageId);
            if (tabIndex >= 0) {
                tabs.currentIndex = tabIndex;
            }
        }

        // Collects every section title inside a page so that the search bar
        // matches the settings themselves and not only the page name.
        function keywordsOf(item, depth) {
            var words = "";
            if (!Boolean(item) || depth > 4) {
                return words;
            }
            if (item.title !== undefined && typeof item.title === "string") {
                words += " " + item.title;
            }
            if (item.text !== undefined && typeof item.text === "string") {
                words += " " + item.text;
            }
            var children = item.children;
            if (Boolean(children)) {
                for (var i = 0; i < children.length; ++i) {
                    words += prv.keywordsOf(children[i], depth + 1);
                }
            }
            return words;
        }

        function matches(page, needle) {
            if (needle === "") {
                return true;
            }
            var haystack = page.title;
            var obj = prv.pagesObjects[page.id];
            if (Boolean(obj)) {
                haystack += prv.keywordsOf(obj, 0);
            }
            var expression = null;
            try {
                expression = new RegExp(needle, "i");
            } catch (error) {
                expression = null;
            }
            if (expression !== null) {
                return expression.test(haystack);
            }
            return haystack.toLowerCase().indexOf(needle.toLowerCase()) !== -1;
        }

        function applyFilter() {
            var result = [];
            for (var i = 0; i < prv.pages.length; ++i) {
                if (prv.matches(prv.pages[i], prv.searchText)) {
                    result.push(prv.pages[i]);
                }
            }
            if (result.length === 0) {
                result = prv.pages;
            }
            prv.filteredPages = result;

            // Keep the shown page inside the filtered set, so a search always
            // lands on something that matches.
            if (prv.indexIn(result, preferencesModel.currentPageId) < 0 && result.length > 0) {
                preferencesModel.currentPageId = result[0].id;
            }

            prv.updateStackCurrentIndex();
        }
    }

    Component.onCompleted: {
        preferencesModel.load(root.currentPageId);

        root.initPagesObjects();

        prv.applyFilter();
        prv.updateStackCurrentIndex();
    }

    function initPagesObjects() {
        var pages = preferencesModel.availablePages();
        var known = [];

        for (var i in pages) {
            var pageInfo = pages[i];

            if (!Boolean(pageInfo.path)) {
                continue;
            }

            var pageComponent = Qt.createComponent("../" + pageInfo.path);

            if (pageComponent.status === Component.Error) {
                console.error("Error loading page:", pageInfo.path, pageComponent.errorString());
                continue;
            }

            var properties = {
                navigationSection: root.navigationSection,
                navigationOrderStart: (Number(i) + 1) * 100
            };

            if (root.currentPageId === pageInfo.id) {
                var params = root.params;
                for (var key in params) {
                    properties[key] = params[key];
                }
            }

            var obj = pageComponent.createObject(stack, properties);

            if (!Boolean(obj)) {
                continue;
            }

            obj.hideRequested.connect(function () {
                root.hide();
            });

            prv.pagesObjects[pageInfo.id] = obj;
            known.push(pageInfo);
        }

        prv.pages = known;
        prv.filteredPages = known;
    }

    PreferencesModel {
        id: preferencesModel

        onCurrentPageIdChanged: function (currentPageId) {
            prv.updateStackCurrentIndex();
        }
    }

    Item {
        width: parent.width
        height: root.contentHeight - 172

        ColumnLayout {
            anchors.fill: parent
            spacing: 16

            M3SearchBar {
                id: searchBar

                Layout.fillWidth: true

                placeholder: qsTrc("preferences", "Search settings")
                showRegexBuilder: true

                navigation.panel: NavigationPanel {
                    name: "PreferencesSearch"
                    section: root.navigationSection
                    order: 0
                }

                onSearchTextChanged: {
                    prv.searchText = searchBar.searchText;
                    prv.applyFilter();
                }

                onRegexBuilderRequested: {
                    root.regexBuilderRequested();
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                spacing: 16

                Flickable {
                    Layout.preferredWidth: 232
                    Layout.fillHeight: true

                    clip: true
                    contentWidth: width
                    contentHeight: tabs.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds

                    ScrollBar.vertical: StyledScrollBar {}

                    M3Tabs {
                        id: tabs

                        width: parent.width
                        height: implicitHeight

                        orientation: Qt.Vertical
                        primary: false

                        model: prv.filteredPages.map(function (page) {
                            return {
                                "text": page.title,
                                "icon": page.icon
                            };
                        })

                        navigationPanel: NavigationPanel {
                            name: "PreferencesPages"
                            section: root.navigationSection
                            direction: NavigationPanel.Vertical
                            order: 1
                        }

                        onActivated: function (index) {
                            var page = prv.filteredPages[index];
                            if (Boolean(page)) {
                                preferencesModel.currentPageId = page.id;
                            }
                        }
                    }
                }

                M3Divider {
                    Layout.fillHeight: true
                    orientation: Qt.Vertical
                }

                StackLayout {
                    id: stack

                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }
        }
    }

    actions: [
        M3Button {
            text: qsTrc("appshell/preferences", "Reset preferences")
            variant: "text"

            onClicked: {
                var pages = preferencesModel.availablePages();

                for (var i in pages) {
                    var obj = prv.pagesObjects[pages[i].id];
                    if (Boolean(obj)) {
                        obj.reset();
                    }
                }

                preferencesModel.resetFactorySettings();
            }
        },
        M3Button {
            text: qsTrc("global", "Cancel")
            variant: "text"

            onClicked: {
                preferencesModel.cancel();
                root.reject();
            }
        },
        M3Button {
            text: qsTrc("global", "OK")
            variant: "filled"

            onClicked: {
                var ok = true;
                var pages = preferencesModel.availablePages();

                for (var i in pages) {
                    var obj = prv.pagesObjects[pages[i].id];
                    if (Boolean(obj)) {
                        ok &= obj.apply();
                    }
                }

                if (ok) {
                    preferencesModel.apply();
                    root.hide();
                }
            }
        }
    ]
}
