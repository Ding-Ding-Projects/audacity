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

import Muse.Ui 1.0
import Muse.UiComponents

import Audacity.M3
import Audacity.AppShell

Item {
    id: root

    property alias appWindow: appMenuModel.appWindow

    property int availableWidth: ui.rootItem.width
    property bool truncated: availableWidth < contentRow.childrenRect.width

    implicitWidth: contentRow.width
    implicitHeight: contentRow.height

    AppMenuModel {
        id: appMenuModel

        appMenuAreaRect: Qt.rect(root.x, root.y, root.width, root.height)
        openedMenuAreaRect: prv.openedArea(menuLoader)

        onOpenMenuRequested: function (menuId) {
            prv.openMenu(menuId);
        }

        onCloseOpenedMenuRequested: {
            menuLoader.close();
        }
    }

    AccessibleItem {
        id: panelAccessibleInfo

        visualItem: root
        role: MUAccessible.Panel
        name: qsTrc("appshell", "Application menu")
    }

    Component.onCompleted: {
        appMenuModel.load();
    }

    Row {
        id: contentRow

        Repeater {
            model: appMenuModel

            delegate: Item {
                id: radioButtonDelegate

                property var item: model ? model.item : null
                property string menuId: Boolean(item) ? item.id : ""
                property string title: Boolean(item) ? item.title : ""
                property string titleWithMnemonicUnderline: Boolean(item) ? item.titleWithMnemonicUnderline : ""

                property bool isMenuOpened: menuLoader.isMenuOpened && menuLoader.parent === this

                property bool highlight: appMenuModel.highlightedMenuId === menuId
                onHighlightChanged: {
                    if (highlight) {
                        forceActiveFocus();
                        accessibleInfo.readInfo();
                    } else {
                        accessibleInfo.resetFocus();
                    }
                }

                property int viewIndex: index

                implicitWidth: textLabel.width + 24
                implicitHeight: M3.density.apply(40)

                visible: mapToItem(root, 0, 0).x + width < root.availableWidth

                AccessibleItem {
                    id: accessibleInfo

                    //! NOTE Menu items stay ignored until they are highlighted,
                    //! which is what the previous button delegate did.
                    ignored: true

                    accessibleParent: panelAccessibleInfo
                    visualItem: radioButtonDelegate
                    role: MUAccessible.Button
                    name: radioButtonDelegate.title

                    function readInfo() {
                        accessibleInfo.ignored = false;
                        accessibleInfo.focused = true;
                    }

                    function resetFocus() {
                        accessibleInfo.ignored = true;
                        accessibleInfo.focused = false;
                    }
                }

                Rectangle {
                    id: background

                    anchors.fill: parent
                    anchors.topMargin: 4
                    anchors.bottomMargin: 4

                    radius: M3.shape.full
                    color: radioButtonDelegate.isMenuOpened ? M3.color.secondaryContainer : "transparent"

                    Behavior on color {
                        ColorAnimation {
                            duration: M3.motion.short3
                            easing: M3.motion.standard
                        }
                    }

                    M3StateLayer {
                        anchors.fill: parent
                        radius: background.radius
                        color: textLabel.color
                        hovered: mouseArea.containsMouse
                        pressed: mouseArea.containsPress
                        focused: radioButtonDelegate.highlight
                    }

                    M3FocusRing {
                        anchors.fill: parent
                        shapeRadius: background.radius
                        visible: radioButtonDelegate.highlight
                    }
                }

                Text {
                    id: textLabel

                    anchors.centerIn: parent

                    width: textMetrics.width

                    text: appMenuModel.isNavigationStarted ? radioButtonDelegate.titleWithMnemonicUnderline : radioButtonDelegate.title
                    textFormat: Text.RichText
                    font: M3.typography.labelLarge
                    color: radioButtonDelegate.isMenuOpened ? M3.color.onSecondaryContainer : M3.color.onSurface

                    TextMetrics {
                        id: textMetrics

                        font: textLabel.font
                        text: radioButtonDelegate.title
                    }
                }

                MouseArea {
                    id: mouseArea

                    anchors.fill: parent
                    hoverEnabled: true

                    onContainsMouseChanged: {
                        if (!mouseArea.containsMouse) {
                            return;
                        }

                        if (menuLoader.isMenuOpened && menuLoader.parent !== radioButtonDelegate) {
                            appMenuModel.openMenu(radioButtonDelegate.menuId, true);
                        }
                    }

                    onClicked: {
                        appMenuModel.openMenu(radioButtonDelegate.menuId, false);
                    }
                }
            }
        }
    }

    StyledMenuLoader {
        id: menuLoader

        property string menuId: ""

        onHandleMenuItem: function (itemId) {
            Qt.callLater(appMenuModel.handleMenuItem, itemId);
        }

        onOpened: {
            appMenuModel.openedMenuId = menuLoader.menuId;
        }

        onClosed: {
            appMenuModel.openedMenuId = "";
        }
    }

    QtObject {
        id: prv

        property var openedMenu: null
        property bool needRestoreNavigationAfterClose: false
        property string lastOpenedMenuId: ""

        function openMenu(menuId, byHover) {
            var children = contentRow.children;
            for (var i = 0; i < children.length; ++i) {
                var item = children[i];
                if (Boolean(item) && item.menuId === menuId) {
                    needRestoreNavigationAfterClose = true;
                    lastOpenedMenuId = menuId;

                    if (!byHover) {
                        if (menuLoader.isMenuOpened && menuLoader.parent === item) {
                            menuLoader.close();
                            return;
                        }
                    }

                    menuLoader.menuId = menuId;
                    menuLoader.parent = item;

                    Qt.callLater(menuLoader.open, item.item.subitems);

                    return;
                }
            }
        }

        function hasNavigatedItem() {
            return appMenuModel.highlightedMenuId !== "";
        }

        function openedArea(menuLoader) {
            if (menuLoader.isMenuOpened) {
                if (menuLoader.menu.subMenuLoader && menuLoader.menu.subMenuLoader.isMenuOpened)
                    return openedArea(menuLoader.menu.subMenuLoader);
                return Qt.rect(menuLoader.menu.x, menuLoader.menu.y, menuLoader.menu.width, menuLoader.menu.height);
            }
            return Qt.rect(0, 0, 0, 0);
        }
    }
}
