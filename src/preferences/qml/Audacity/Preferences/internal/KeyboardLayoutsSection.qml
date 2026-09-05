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

import Muse.Ui 1.0
import Muse.UiComponents

import Audacity.UiComponents 1.0
import Audacity.M3

BaseSection {
    id: root

    title: qsTrc("appshell/preferences", "Keyboard layout")

    navigation.direction: NavigationPanel.Horizontal

    property alias keyboardLayouts: dropdown.model
    property string currentKeyboardLayout: ""

    signal keyboardLayoutSelected(string keyboardLayout)

    M3Dropdown {
        id: dropdown
        function indexOfValue(value) {
            var items = dropdown.model
            for (var i = 0; i < items.length; ++i) {
                var item = items[i]
                var candidate = (typeof item === "object" && item !== null) ? item[dropdown.valueRole] : item
                if (candidate === value) {
                    return i
                }
            }
            return -1
        }
        width: root.columnWidth

        currentIndex: dropdown.indexOfValue(root.currentKeyboardLayout)

        navigation.name: "LanguagesBox"
        navigation.accessible.name: qsTrc("appshell/preferences", "Keyboard layout %1").arg(currentText)
        navigation.panel: root.navigation
        navigation.order: 1

        onActivated: function (index, value) {
            root.keyboardLayoutSelected(value)
        }
    }
}
