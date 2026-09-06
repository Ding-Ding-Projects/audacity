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
import QtQuick 2.15
import QtQuick.Layouts 1.15

import Muse.Ui 1.0
import Muse.UiComponents
import Audacity.M3

ColumnLayout {
    id: root

    property alias title: titleLabel.text
    property alias description: descriptionLabel.text
    property alias buttonText: button.text
    property alias buttonEnabled: button.enabled

    property alias imageSource: image.source

    property alias navigation: button.navigation

    signal buttonClicked

    readonly property real radius: M3.shape.small

    spacing: 0

    RoundedRectangle {
        Layout.fillWidth: true
        implicitHeight: 208

        color: ui.theme.extra["save_option_background_color"]

        topLeftRadius: root.radius
        topRightRadius: root.radius

        Image {
            id: image
            anchors.fill: parent

            fillMode: Image.PreserveAspectFit
        }
    }

    RoundedRectangle {
        Layout.fillWidth: true
        Layout.fillHeight: true

        color: M3.color.surfaceContainer

        bottomLeftRadius: root.radius
        bottomRightRadius: root.radius

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 24
            spacing: 24

            StyledTextLabel {
                id: titleLabel
                Layout.fillWidth: true
                font: M3.typography.headlineSmall
                horizontalAlignment: Text.AlignLeft
            }

            StyledTextLabel {
                id: descriptionLabel
                Layout.fillWidth: true
                Layout.fillHeight: true
                wrapMode: Text.WordWrap
                maximumLineCount: 0
                font: M3.typography.bodyLarge
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignTop
            }

            M3Button {
                id: button
                variant: "filled"

                onClicked: {
                    root.buttonClicked()
                }
            }
        }
    }
}
