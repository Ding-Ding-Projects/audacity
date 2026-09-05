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

import Muse.UiComponents
import Audacity.M3

Column {
    id: root

    property alias title: titleLabel.text
    property alias elide: titleLabel.elide
    property alias wrapMode: titleLabel.wrapMode

    property real columnWidth: 208

    property real titleHeight: -1
    readonly property alias titleImplicitHeight: titleLabel.implicitHeight

    property alias currentIndex: comboBox.currentIndex
    property alias currentValue: comboBox.currentValue
    property alias textRole: comboBox.textRole
    property alias valueRole: comboBox.valueRole
    property alias model: comboBox.model
    property alias control: comboBox

    property alias navigation: comboBox.navigation

    signal valueEdited(int newIndex, var newValue)

    spacing: 6

    function indexOfValue(value) {
        var items = comboBox.model;
        if (!items) {
            return -1;
        }
        for (var i = 0; i < items.length; ++i) {
            var item = items[i];
            var candidate = (typeof item === "object" && item !== null) ? item[comboBox.valueRole] : item;
            if (candidate === value) {
                return i;
            }
        }
        return -1;
    }

    StyledTextLabel {
        id: titleLabel

        width: root.columnWidth
        height: root.titleHeight >= 0 ? root.titleHeight : implicitHeight

        horizontalAlignment: Qt.AlignLeft
        verticalAlignment: Text.AlignTop
        wrapMode: Text.WordWrap
        maximumLineCount: 2
    }

    M3Dropdown {
        id: comboBox

        width: root.columnWidth

        navigation.accessible.name: root.title + " " + currentText

        onActivated: function (index, value) {
            root.valueEdited(index, value);
        }
    }
}
