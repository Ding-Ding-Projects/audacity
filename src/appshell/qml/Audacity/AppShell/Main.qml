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
 * The default application window, used where no platform specific window is
 * built.
 */
import QtQuick
import QtQuick.Window

import Muse.UiComponents

import Audacity.AppShell

AppWindow {
    id: root

    flags: Qt.Window | Qt.FramelessWindowHint

    function toggleMaximized() {
        if (root.visibility === Window.Maximized) {
            root.showNormal()
        } else {
            root.showMaximized()
        }
    }

    FramelessWindowModel {
        id: framelessWindowModel

        titleBarMoveArea: appTitleBar.titleMoveAreaRect
    }

    Component.onCompleted: {
        framelessWindowModel.init()
        window.init()
    }

    M3AppTitleBar {
        id: appTitleBar

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        title: root.title
        windowVisibility: root.visibility
        appWindow: root
        handleWindowGestures: true

        onShowWindowMinimizedRequested: {
            root.showMinimized()
        }

        onToggleWindowMaximizedRequested: {
            root.toggleMaximized()
        }

        onCloseWindowRequested: {
            root.close()
        }
    }

    WindowContent {
        id: window

        anchors.top: appTitleBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
