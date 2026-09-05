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
 * The developer tools navigation, drawn as Material 3 list items.
 */
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

ListView {
    id: radioButtonList

    orientation: ListView.Vertical
    spacing: 2

    signal selected(string name)

    currentIndex: 0

    NavigationPanel {
        id: navPanel
        name: "DevToolsMenu"
        enabled: radioButtonList.enabled && radioButtonList.visible
        direction: NavigationPanel.Vertical
        accessible.name: "DevTools menu"
    }

    delegate: M3ListItem {
        id: listItem

        required property int index
        required property var modelData

        width: ListView.view.width

        headline: listItem.modelData["title"]
        selected: listItem.index === radioButtonList.currentIndex

        navigation.panel: navPanel
        navigation.row: listItem.index

        onClicked: {
            radioButtonList.currentIndex = listItem.index;
            radioButtonList.selected(listItem.modelData["name"]);
        }
    }
}
