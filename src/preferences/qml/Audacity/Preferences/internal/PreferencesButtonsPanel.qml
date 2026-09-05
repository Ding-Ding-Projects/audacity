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
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3

Rectangle {
    id: root

    color: M3.color.surface

    property NavigationPanel navigation: NavigationPanel {
        name: "PreferencesButtons"
        direction: NavigationPanel.Horizontal
        enabled: root.enabled && root.visible
    }

    signal revertFactorySettingsRequested
    signal applyRequested
    signal rejectRequested

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.margins: 24

        spacing: 8

        M3Button {
            text: qsTrc("appshell/preferences", "Reset preferences")
            variant: "text"

            navigation.panel: root.navigation
            navigation.column: 0

            onClicked: root.revertFactorySettingsRequested()
        }

        Item {
            Layout.fillWidth: true
        }

        M3Button {
            text: qsTrc("global", "Cancel")
            variant: "text"

            navigation.panel: root.navigation
            navigation.column: 1

            onClicked: root.rejectRequested()
        }

        M3Button {
            text: qsTrc("global", "OK")
            variant: "filled"

            navigation.panel: root.navigation
            navigation.column: 2

            onClicked: root.applyRequested()
        }
    }
}
