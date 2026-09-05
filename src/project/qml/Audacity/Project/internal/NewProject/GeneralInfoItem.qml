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
import QtQuick 2.9

import Muse.Ui 1.0
import Muse.UiComponents
import Audacity.M3

Column {
    id: root

    property string title: ""
    property alias info: textField.placeholder

    property alias navigation: textField.navigation

    spacing: 10

    StyledTextLabel {
        anchors.left: parent.left
        anchors.right: parent.right

        font: M3.typography.titleSmall
        horizontalAlignment: Text.AlignLeft
        text: title
    }

    M3TextField {
        id: textField

        navigation.accessible.name: root.title + " " + currentText

        onTextEdited: function (newTextValue) {
            root.info = newTextValue;
        }
    }
}
