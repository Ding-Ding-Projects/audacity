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

import Muse.Shortcuts 1.0

import Audacity.AppShell
import Audacity.Companion

PreferencesPage {
    id: root

    contentFillsAvailableHeight: true

    property alias shortcutCodeKey: page.shortcutCodeKey

    function apply() {
        return page.apply()
    }

    function reset() {
        page.reset()
    }

    ShortcutsPage {
        id: page

        anchors.fill: parent

        navigationSection: root.navigationSection
        navigationOrderStart: root.navigationOrderStart

        onRegexBuilderRequested: {
            regexBuilder.open()
        }
    }

    // The regular expression builder for the shortcut search field. Each search
    // surface owns its own instance, so its pattern, flags, sample and saved
    // test cases are isolated from every other search field in the application.
    RegexBuilderSheet {
        id: regexBuilder

        anchors.fill: parent

        storeName: "shortcuts-preferences"
        fieldLabel: qsTrc("shortcuts", "Search shortcut")

        onPatternAccepted: function (pattern) {
            page.setSearchText(pattern)
        }
    }
}
