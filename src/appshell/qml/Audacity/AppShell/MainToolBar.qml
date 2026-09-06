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

/*
 * The main page switcher, drawn as a browser style tab strip.
 *
 * The strip carries the fixed application pages, one tab per open project and
 * one tab per dockable panel. Page switching still goes through the same
 * selected(uri) signal the window content listens to, so nothing about the
 * navigation between pages changed.
 */
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.AppShell
import Audacity.Chronicle

Item {
    id: root

    width: tabStrip.implicitWidth > 0 ? tabStrip.implicitWidth : 640
    height: M3.density.apply(48)

    // The dock toolbar host sizes this item from its implicit size, not its
    // explicit width, so without these the strip is laid out at whatever
    // narrow default the host picks and every tab overflows into the "more"
    // button.
    implicitWidth: root.width
    implicitHeight: root.height

    property alias navigation: navPanel

    property string currentUri: "audacity://home"

    // The dockable panels the strip should also offer as tabs, as a list of
    // { id, title, uri }.
    property var dockPanels: []

    signal selected(string uri)

    function select(uri) {
        root.selected(uri)
    }

    function focusOnFirst() {
        navPanel.requestActive()
    }

    MainToolBarModel {
        id: toolBarModel
    }

    Component.onCompleted: {
        toolBarModel.load()
    }

    NavigationPanel {
        id: navPanel
        name: "MainToolBar"
        enabled: root.enabled && root.visible
        accessible.name: qsTrc("appshell", "Main toolbar")
    }

    Rectangle {
        anchors.fill: parent
        color: M3.color.surface
    }

    readonly property var tabSources: {
        var items = [];

        // The fixed application pages. They can be reordered, pinned and
        // grouped like any other tab, but never closed.
        for (var i = 0; i < toolBarModel.items.length; ++i) {
            var page = toolBarModel.items[i]
            items.push({
                "id": String(page.uri),
                "title": String(page.title),
                "kind": "page",
                "uri": String(page.uri),
                "icon": IconCode.NONE,
                "closable": false
            })
        }

        // One tab per dockable panel the host offers.
        for (var j = 0; j < root.dockPanels.length; ++j) {
            var panel = root.dockPanels[j]
            items.push({
                "id": "panel:" + String(panel.id),
                "title": String(panel.title),
                "kind": "panel",
                "uri": panel.uri !== undefined ? String(panel.uri) : "",
                "icon": IconCode.NONE,
                "closable": true
            })
        }

        return items
    }

    TabStrip {
        id: tabStrip

        anchors.fill: parent

        surfaceId: "main"
        defaultDockSide: "top"
        sources: root.tabSources
        currentTabId: root.currentUri

        navigationPanel.section: navPanel.section
        navigationPanel.order: navPanel.order

        onTabActivated: function (id, uri) {
            if (uri !== "") {
                root.selected(uri)
            }
        }
    }

    M3Divider {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
