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
 * The theme picker. Every sample is a Material 3 card with a Material 3 radio
 * button under it.
 */
pragma ComponentBehavior: Bound

import QtQuick

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.AppShell

ListView {
    id: root

    property alias themes: root.model
    property string currentThemeCode

    currentIndex: model.findIndex(theme => theme.codeKey === currentThemeCode)

    property NavigationPanel navigationPanel: NavigationPanel {
        name: "ThemeSamplesList"
        enabled: root.enabled && root.visible
        direction: NavigationPanel.Horizontal

        onNavigationEvent: function (event) {
            if (event.type === NavigationEvent.AboutActive) {
                event.setData("controlIndex", [navigationRow, navigationColumnStart + root.currentIndex])
            }
        }
    }

    property int navigationRow: -1
    property int navigationColumnStart: 0
    readonly property int navigationColumnEnd: navigationColumnStart + count

    signal themeChangeRequested(var newThemeCode)

    readonly property int sampleWidth: 88
    readonly property int sampleHeight: 108

    implicitWidth: count * sampleWidth + (count - 1) * spacing
    height: contentHeight
    contentHeight: sampleHeight

    orientation: Qt.Horizontal
    interactive: false

    spacing: 24

    delegate: Column {
        id: sampleColumn

        required property int index
        required property var modelData

        width: root.sampleWidth
        height: root.sampleHeight

        spacing: 10

        M3Card {
            width: parent.width
            height: 64

            variant: "outlined"
            padding: 0
            clickable: true

            accessibleName: qsTrc("appshell/gettingstarted", "%1 theme").arg(sampleColumn.modelData.title)

            onClicked: {
                root.themeChangeRequested(sampleColumn.modelData.codeKey)
            }

            ThemeSample {
                anchors.fill: parent

                theme: sampleColumn.modelData

                onClicked: {
                    root.themeChangeRequested(sampleColumn.modelData.codeKey)
                }
            }
        }

        M3RadioButton {
            width: parent.width

            checked: root.currentThemeCode === sampleColumn.modelData.codeKey
            text: sampleColumn.modelData.title

            navigation.name: text
            navigation.panel: root.navigationPanel
            navigation.row: root.navigationRow
            navigation.column: root.navigationColumnStart + sampleColumn.index
            //: %1 is the theme name (e.g. "Light", "Dark")
            navigation.accessible.name: qsTrc("appshell/gettingstarted", "%1 theme").arg(sampleColumn.modelData.title)
            navigation.accessible.description: {
                //: %1 is the theme name (e.g. "Light", "Dark")
                var desc = qsTrc("appshell/gettingstarted", "Select %1 theme").arg(sampleColumn.modelData.title)
                if (root.currentThemeCode === sampleColumn.modelData.codeKey) {
                    //: %1 is the base description with theme selection
                    desc = qsTrc("appshell/gettingstarted", "%1. Currently selected").arg(desc)
                }
                return desc
            }

            onToggled: {
                root.themeChangeRequested(sampleColumn.modelData.codeKey)
            }
        }
    }
}
