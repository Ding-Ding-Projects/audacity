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
 * The Material 3 seed colour page. Every colour in the interface is generated
 * from the seed chosen here.
 */
import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.AppShell

Page {
    id: root

    title: qsTrc("appshell/gettingstarted", "Choose your colour")

    titleContentSpacing: 16

    property NavigationPanel pickerPanel: NavigationPanel {
        name: "SeedColorPanel"
        enabled: root.enabled && root.visible
        section: root.navigationSection
        order: root.navigationStartRow + 1
        direction: NavigationPanel.Both
        accessible.name: qsTrc("appshell/gettingstarted", "Seed colour")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24

        spacing: 16

        Text {
            Layout.fillWidth: true

            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap

            text: qsTrc("appshell/gettingstarted", "The whole palette is generated from one seed colour. You can change it later in preferences.")
            font: M3.typography.bodyMedium
            color: M3.color.onSurfaceVariant
        }

        M3ColorPicker {
            id: picker

            Layout.fillWidth: true
            Layout.fillHeight: true

            allowRainbow: false
            selection: M3.seedColor.toString()
            contrastBackground: M3.color.surface

            navigationPanel: root.pickerPanel

            onSelectionChanged: {
                if (picker.selection !== "" && picker.selection !== "rainbow") {
                    M3.seedColor = picker.selection
                }
            }
        }
    }
}
