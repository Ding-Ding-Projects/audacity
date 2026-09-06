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

    // The running version comes from the application, rather than a separate
    // display-only build definition. It therefore includes the build suffix
    // users need when reporting a problem.
    readonly property string runningVersionLine: qsTrc("appshell", "Version %1").arg(aboutModel.appVersion())

    // AboutModel reads AU_BUILD_TIMESTAMP_UTC, fixed when CMake configured
    // this artifact. It deliberately has no launch-time or file-time fallback.
    readonly property string buildProvenanceLine: {
        var local = aboutModel.buildUpdatedAtLocal()
        if (local === "") {
            return qsTrc("appshell", "Build provenance unavailable")
        }
        return qsTrc("appshell", "Version source updated at %1").arg(local)
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
        anchors.bottomMargin: narrowProvenance.implicitHeight + 20

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

        // Version and provenance belong on the first visible screen. Keep the
        // full lines wrapped instead of eliding a factual value.
        Column {
            Layout.fillWidth: true
            Layout.bottomMargin: 12

            spacing: 2

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                accessible.name: root.runningVersionLine
                text: root.runningVersionLine
                font: M3.typography.labelLarge
                color: M3.color.onSurface
            }

            StyledTextLabel {
                width: parent.width
                horizontalAlignment: Text.AlignLeft
                wrapMode: Text.WordWrap
                accessible.name: root.buildProvenanceLine
                text: root.buildProvenanceLine
                font: M3.typography.bodySmall
                color: M3.color.onSurfaceVariant
            }
        }
    }

    // A narrow window still presents the complete values before navigation.
    // Word wrapping is intentional: factual build provenance must not be
    // shortened to fit the collapsed rail.
    Column {
        id: narrowProvenance
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 8

        visible: root.iconsOnly

        spacing: 2

        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WrapAnywhere
            accessible.name: root.runningVersionLine
            text: root.runningVersionLine
            font: M3.typography.labelSmall
            color: M3.color.onSurfaceVariant
        }

        StyledTextLabel {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WrapAnywhere
            accessible.name: root.buildProvenanceLine
            text: root.buildProvenanceLine
            font: M3.typography.labelSmall
            color: M3.color.onSurfaceVariant
        }
    }
}
