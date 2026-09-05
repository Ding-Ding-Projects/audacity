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

Rectangle {
    id: root

    property alias navigation: navPanel

    width: view.width
    height: view.height

    color: M3.color.surfaceContainer

    NavigationPanel {
        id: navPanel
        name: "PublishToolBar"
        enabled: root.enabled && root.visible
    }

    PublishToolBarModel {
        id: toolBarModel
    }

    Component.onCompleted: {
        toolBarModel.load();
    }

    ListView {
        id: view

        height: contentItem.childrenRect.height
        width: contentItem.childrenRect.width

        orientation: Qt.Horizontal
        interactive: false

        spacing: 8

        model: toolBarModel

        delegate: M3Button {
            property var item: Boolean(model) ? model.item : null

            text: Boolean(item) ? item.title : ""
            icon: Boolean(item) ? item.icon : IconCode.NONE
            enabled: Boolean(item) ? item.enabled : false
            toolTipTitle: Boolean(item) ? item.title : ""
            toolTipDescription: Boolean(item) ? item.description : ""
            toolTipShortcut: Boolean(item) ? item.shortcuts : ""

            variant: "text"

            navigation.panel: navPanel
            navigation.order: model.index

            height: 40

            onClicked: {
                toolBarModel.handleMenuItem(item.id);
            }
        }
    }
}
