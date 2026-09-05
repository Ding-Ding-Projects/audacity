/*
 * SPDX-License-Identifier: GPL-3.0-only
 * Audacity-CLA-applies
 *
 * Audacity
 * Music Composition & Notation
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

/*
 * The home page navigation, drawn as a Material 3 navigation rail when the
 * panel is collapsed and as a Material 3 navigation list when it is expanded.
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.AppShell

Item {
    id: root

    property string currentPageName: ""
    property bool iconsOnly: false
    property bool cloudEnabled: false

    signal selected(string name)

    // Build provenance for the front screen. Both values are fixed when the
    // build is configured, so this line never reports the time the
    // application was started.
    AboutModel {
        id: aboutModel
    }

    readonly property string buildVersionLine: qsTrc("appshell", "Material Audacity %1").arg(aboutModel.buildVersion())

    readonly property string buildUpdatedLine: {
        var local = aboutModel.buildUpdatedAtLocal()
        if (local === "") {
            return qsTrc("appshell", "Build time unavailable")
        }
        return qsTrc("appshell", "Updated %1").arg(local)
    }

    readonly property var destinations: {
        var items = []

        if (root.cloudEnabled) {
            items.push({
                "name": "account",
                "title": qsTrc("appshell", "Cloud account"),
                "text": qsTrc("appshell", "Cloud account"),
                "icon": IconCode.ACCOUNT
            })
        }

        items.push({
            "name": "projects",
            "title": qsTrc("appshell", "Project"),
            "text": qsTrc("appshell", "Project"),
            "icon": IconCode.NEW_FILE
        })

        return items
    }

    function indexOfCurrent() {
        for (var i = 0; i < root.destinations.length; ++i) {
            if (root.destinations[i].name === root.currentPageName) {
                return i
            }
        }

        return 0
    }

    NavigationSection {
        id: navSec
        name: "HomeMenuSection"
        enabled: root.enabled && root.visible
        order: 2
    }

    NavigationPanel {
        id: navPanel
        name: "HomeMenuPanel"
        enabled: root.enabled && root.visible
        section: navSec
        order: 1
        direction: NavigationPanel.Vertical

        accessible.name: qsTrc("appshell", "Home menu") + " " + navPanel.directionInfo
    }

    Rectangle {
        anchors.fill: parent
        color: M3.color.surfaceContainer
    }

    M3NavigationRail {
        id: rail

        anchors.fill: parent
        anchors.topMargin: 12

        visible: root.iconsOnly

        model: root.destinations
        currentIndex: root.indexOfCurrent()
        showLabels: false

        navigationPanel: navPanel

        onActivated: function (index) {
            root.selected(root.destinations[index].name)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 12
        anchors.leftMargin: 12
        anchors.rightMargin: 12

        spacing: 4

        visible: !root.iconsOnly

        Repeater {
            model: root.destinations

            delegate: M3ListItem {
                id: destination

                required property int index
                required property var modelData

                Layout.fillWidth: true

                headline: destination.modelData.title
                leadingIcon: destination.modelData.icon
                selected: destination.modelData.name === root.currentPageName

                navigation.panel: navPanel
                navigation.row: destination.index

                onClicked: {
                    root.selected(destination.modelData.name)
                }
            }
        }

        Item {
            Layout.fillHeight: true
            Layout.fillWidth: true
        }

        // The version and the build time, on the front screen rather than
        // hidden away in the about dialog.
        Column {
            Layout.fillWidth: true
            Layout.bottomMargin: 12

            spacing: 2

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                elide: Text.ElideRight
                text: root.buildVersionLine
                font: M3.typography.labelLarge
                color: M3.color.onSurface
            }

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                text: root.buildUpdatedLine
                font: M3.typography.bodySmall
                color: M3.color.onSurfaceVariant
            }
        }
    }

    // Just the version when the rail is collapsed to icons and there is no
    // room for the full two lines.
    StyledTextLabel {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8

        visible: root.iconsOnly
        horizontalAlignment: Text.AlignHCenter
        elide: Text.ElideRight
        text: aboutModel.buildVersion()
        font: M3.typography.labelSmall
        color: M3.color.onSurfaceVariant
    }
}
