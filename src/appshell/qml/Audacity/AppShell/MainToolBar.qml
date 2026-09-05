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
 * The main page switcher, drawn as Material 3 primary tabs.
 */
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.AppShell

Item {
    id: root

    width: tabsRow.width
    height: M3.density.apply(48)

    property alias navigation: navPanel

    property string currentUri: "audacity://home"

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

    Row {
        id: tabsRow

        height: parent.height

        Repeater {
            model: toolBarModel.items

            delegate: M3Tab {
                id: tab

                required property int index
                required property var modelData

                height: tabsRow.height

                primary: true
                enabled: modelData.enabled
                selected: modelData.uri === root.currentUri
                text: modelData.title
                accessibleName: modelData.title

                navigation.panel: navPanel
                navigation.column: tab.index

                onClicked: {
                    root.selected(tab.modelData.uri)
                }
            }
        }
    }

    M3Divider {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
