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
import QtQuick
import QtQuick.Controls

import Muse.Dock
import Muse.Interactive
import Muse.Ui
import Muse.UiComponents

import Audacity.AppShell
import Audacity.Companion
import Audacity.Experience
import Audacity.SquirrelUpdate

DockWindow {
    id: root

    objectName: "WindowContent"

    property var interactiveProvider: InteractiveProvider {
        topParent: root

        onRequestedDockPage: function (uri, params) {
            root.loadPage(uri, params)
        }
    }

    onPageLoaded: {
        root.interactiveProvider.onPageOpened()
        window.opacity = 1.0
    }

    property NavigationSection topToolKeyNavSec: NavigationSection {
        id: keynavSec
        name: "TopTool"
        order: 1
    }

    toolBars: [
        DockToolBar {
            id: mainToolBar

            objectName: "mainToolBar"
            title: qsTrc("appshell", "Main toolbar")

            floatable: false
            closable: false

            MainToolBar {
                id: toolBar
                navigation.section: root.topToolKeyNavSec
                navigation.order: 1

                currentUri: root.currentPageUri

                // The dockable panels the tab strip offers alongside the
                // application pages.
                dockPanels: [
                    {
                        "id": "tracks",
                        "title": qsTrc("appshell", "Tracks"),
                        "uri": ""
                    },
                    {
                        "id": "history",
                        "title": qsTrc("appshell", "History"),
                        "uri": ""
                    },
                    {
                        "id": "effects",
                        "title": qsTrc("appshell", "Effects"),
                        "uri": ""
                    }
                ]

                navigation.onActiveChanged: {
                    if (navigation.active) {
                        mainToolBar.forceActiveFocus()
                    }
                }

                onSelected: function (uri) {
                    api.launcher.open(uri)
                }

                Component.onCompleted: {
                    toolBar.focusOnFirst()
                }
            }
        }
    ]

    pages: [
        HomePage {
            window: root.window
        },
        ProjectPage {
            topToolKeyNavSec: root.topToolKeyNavSec
        },
        PublishPage {
            topToolKeyNavSec: root.topToolKeyNavSec
        },
        DevToolsPage {}
    ]

    // The command palette overlay: Ctrl+Shift+F opens it from anywhere in the
    // application. It stays inert and invisible until first opened.
    CommandPaletteHost {
        id: commandPaletteHost

        z: 1000

        Component.onCompleted: {
            // The window's own pages, so the palette can teleport straight to
            // one without waiting for the tab strip to say what it holds.
            commandPaletteHost.setContextRows([
                {
                    "title": qsTrc("appshell", "Home"),
                    "section": qsTrc("appshell", "Window"),
                    "payload": {
                        "uri": "audacity://home"
                    }
                },
                {
                    "title": qsTrc("appshell", "Tracks"),
                    "section": qsTrc("appshell", "Window"),
                    "payload": {
                        "uri": "audacity://project"
                    }
                },
                {
                    "title": qsTrc("appshell", "Publish"),
                    "section": qsTrc("appshell", "Window"),
                    "payload": {
                        "uri": "audacity://publish"
                    }
                }
            ])
        }
    }

    // The companion surfaces: language, funny level and attention support
    // settings feed a toast stack, a notification centre and the super
    // confirmation gate that guards destructive actions everywhere else in
    // the application.
    ExperienceOverlay {
        id: experienceOverlay

        anchors.fill: parent
        z: 900
    }

    // The unsigned Squirrel.Windows update banner. It stays hidden until a
    // package has downloaded and verified, then sits in the bottom right
    // corner without interrupting whatever the user is doing. Setting
    // AU_SQUIRREL_DEMO_BANNER=1 forces it visible for screenshots and manual
    // review, with a made up version so no real network access is needed.
    SquirrelUpdateModel {
        id: squirrelUpdateModel

        Component.onCompleted: squirrelUpdateModel.load()
    }

    UpdateBanner {
        id: squirrelUpdateBanner

        model: squirrelUpdateModel

        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 16
        z: 950

        visible: squirrelUpdateModel.bannerVisible
    }
}
