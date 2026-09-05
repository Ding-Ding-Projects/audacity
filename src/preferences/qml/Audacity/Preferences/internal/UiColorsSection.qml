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
import QtQuick.Layouts 1.15

import Muse.Ui 1.0
import Muse.UiComponents

import Audacity.UiComponents 1.0
import Audacity.AppShell
import Audacity.M3

BaseSection {
    id: root

    title: qsTrc("preferences", "UI colors")
    navigation.direction: NavigationPanel.Both

    signal colorChangeRequested(var newColor, var propertyType)

    GridLayout {
        id: grid
        width: parent.width

        columnSpacing: root.columnSpacing
        rowSpacing: root.rowSpacing
        columns: 2

        Repeater {
            model: [
                {
                    textRole: qsTrc("preferences", "Accent color"),
                    colorRole: M3.color.primary,
                    typeRole: AppearancePreferencesModel.AccentColor
                },
                {
                    textRole: qsTrc("preferences", "Text and icons"),
                    colorRole: M3.color.onSurface,
                    typeRole: AppearancePreferencesModel.TextAndIconsColor
                },
                {
                    textRole: qsTrc("preferences", "Disabled text"),
                    colorRole: ui.theme.extra["black_color"],
                    typeRole: AppearancePreferencesModel.DisabledColor
                },
                {
                    textRole: qsTrc("preferences", "Border color"),
                    colorRole: M3.color.outlineVariant,
                    typeRole: AppearancePreferencesModel.BorderColor
                }
            ]

            delegate: Row {
                Layout.preferredWidth: (grid.width - grid.columnSpacing) / 2
                spacing: root.columnSpacing

                StyledTextLabel {
                    id: titleLabel
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData["textRole"]
                    width: root.columnWidth / 2
                    horizontalAlignment: Text.AlignLeft
                }

                ColorPicker {
                    width: 112
                    color: modelData["colorRole"]

                    navigation.name: titleLabel.text
                    navigation.panel: root.navigation
                    navigation.row: index / grid.columns
                    navigation.column: index % grid.columns
                    navigation.accessible.name: titleLabel.text

                    onNewColorSelected: function (newColor) {
                        root.colorChangeRequested(newColor, modelData.typeRole)
                    }
                }
            }
        }
    }
}
