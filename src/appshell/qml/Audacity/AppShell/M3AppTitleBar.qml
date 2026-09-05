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
 * The Material 3 top app bar used as the frameless window title bar on every
 * platform. It carries the application mark, the application menu, the window
 * title and the window controls drawn as Material 3 icon buttons.
 */
import QtQuick
import QtQuick.Layouts
import QtQuick.Window

import Muse.Ui
import Muse.UiComponents

import Audacity.M3
import Audacity.AppShell

Rectangle {
    id: root

    property string title: ""

    // The area the platform frameless controller may use to move the window.
    property rect titleMoveAreaRect: Qt.rect(titleMoveArea.x, titleMoveArea.y, titleMoveArea.width, titleMoveArea.height)

    property int windowVisibility: Window.Windowed

    // Shows the in window application menu. Left off where the platform draws
    // its own menu bar.
    property bool showAppMenu: true

    property alias appWindow: menuLoader.appWindow

    // Drives the window move and the double click to maximise gesture when the
    // platform has no frameless window controller of its own.
    property bool handleWindowGestures: false

    signal showWindowMinimizedRequested()
    signal toggleWindowMaximizedRequested()
    signal closeWindowRequested()

    color: M3.color.surfaceContainer

    implicitHeight: M3.density.apply(56)

    readonly property bool windowIsMaximized: root.windowVisibility === Window.Maximized
    readonly property bool showsWindowControls: root.windowVisibility !== Window.FullScreen

    NavigationPanel {
        id: navPanel

        name: "AppTitleBar"
        enabled: root.enabled && root.visible
        order: 0
        accessible.name: qsTrc("appshell", "Title bar")
    }

    RowLayout {
        id: content

        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8

        spacing: 8

        // The application mark.
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32

            radius: M3.shape.small
            color: M3.color.primaryContainer

            StyledIconLabel {
                anchors.centerIn: parent
                iconCode: IconCode.AUDIO
                color: M3.color.onPrimaryContainer
                font.pixelSize: 20
            }
        }

        Loader {
            id: menuLoader

            property var appWindow: null

            Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            Layout.fillWidth: item ? item.truncated : false
            Layout.preferredWidth: item ? item.implicitWidth : 0
            Layout.preferredHeight: item ? item.implicitHeight : 0

            active: root.showAppMenu
            source: root.showAppMenu ? "platform/AppMenuBar.qml" : ""

            onLoaded: {
                item.appWindow = Qt.binding(function () {
                    return menuLoader.appWindow
                })
                item.availableWidth = Qt.binding(function () {
                    return Math.max(0, root.width - 240)
                })
            }
        }

        Item {
            id: titleMoveArea

            Layout.fillWidth: true
            Layout.fillHeight: true

            Text {
                id: titleLabel

                anchors.fill: parent

                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight

                text: root.title
                font: M3.typography.titleMedium
                color: M3.color.onSurface

                visible: root.showsWindowControls
            }

            DragHandler {
                enabled: root.handleWindowGestures
                target: null
                grabPermissions: PointerHandler.CanTakeOverFromAnything

                onActiveChanged: {
                    if (active && root.appWindow) {
                        root.appWindow.startSystemMove()
                    }
                }
            }

            TapHandler {
                enabled: root.handleWindowGestures

                onDoubleTapped: {
                    root.toggleWindowMaximizedRequested()
                }
            }
        }

        Row {
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter

            spacing: 4
            visible: root.showsWindowControls

            M3IconButton {
                icon: IconCode.APP_MINIMIZE
                accessibleName: qsTrc("appshell", "Minimise")
                toolTipTitle: qsTrc("appshell", "Minimise")

                navigation.panel: navPanel
                navigation.order: 1

                onClicked: {
                    root.showWindowMinimizedRequested()
                }
            }

            M3IconButton {
                icon: root.windowIsMaximized ? IconCode.APP_UNMAXIMIZE : IconCode.APP_MAXIMIZE
                accessibleName: root.windowIsMaximized ? qsTrc("appshell", "Restore") : qsTrc("appshell", "Maximise")
                toolTipTitle: accessibleName

                navigation.panel: navPanel
                navigation.order: 2

                onClicked: {
                    root.toggleWindowMaximizedRequested()
                }
            }

            M3IconButton {
                icon: IconCode.APP_CLOSE
                accessibleName: qsTrc("appshell", "Close window")
                toolTipTitle: accessibleName

                navigation.panel: navPanel
                navigation.order: 3

                onClicked: {
                    root.closeWindowRequested()
                }
            }
        }
    }

    M3Divider {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
    }
}
